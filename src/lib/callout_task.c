/* %COPYRIGHT% */





/**
 * General lock order:
 *
 *	The global lock (s_lock_global)
 *	The timed task queue lock (s_lock_task_q)
 *	A task lock (s_lock_task)
 *
 * Note taht the task locks are recursive.
 */





static inline void
s_lock_task(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)lagopus_mutex_lock(&(t->m_lock));
  }
}


static inline void
s_unlock_task(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)lagopus_mutex_unlock(&(t->m_lock));
  }
}





static inline callout_task_state_t
s_get_task_state_in_table(const lagopus_callout_task_t t) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    lagopus_result_t r;

    r = lagopus_hashmap_find(&s_tsk_tbl, (void *)t, &val);
    if (likely(r == LAGOPUS_RESULT_OK)) {
      ret = (callout_task_state_t)val;
    }
  }

  return ret;
}


static inline callout_task_state_t
s_set_task_state_in_table(const lagopus_callout_task_t t,
                          callout_task_state_t s) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    lagopus_result_t r;

    if (likely((r = lagopus_hashmap_find(&s_tsk_tbl, (void *)t, &val)) ==
               LAGOPUS_RESULT_OK)) {
      ret = (callout_task_state_t)val;
      val = (void *)s;

      if (unlikely((r != lagopus_hashmap_add(&s_tsk_tbl, (void *)t,
                                             &val, true)) ==
                   LAGOPUS_RESULT_OK)) {
        ret = TASK_STATE_UNKNOWN;
      }
    } else if (likely(r == LAGOPUS_RESULT_NOT_FOUND)) {
      val = (void *)s;
      if (likely((r = lagopus_hashmap_add(&s_tsk_tbl, (void *)t,
                                          &val, true)) ==
                 LAGOPUS_RESULT_OK)) {
        ret = s;
      }
    }
  }

  return ret;
}


static inline void
s_delete_task_in_table(const lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)lagopus_hashmap_delete(&s_tsk_tbl, (void *)t, NULL, true);
  }
}





static inline callout_task_state_t
s_get_task_state(const lagopus_callout_task_t t) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    
    s_lock_task(t);
    {
      ret = t->m_status;
    }
    s_unlock_task(t);
  }

  return ret;
}


static inline callout_task_state_t
s_set_task_state(const lagopus_callout_task_t t,
                 callout_task_state_t s) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {

    s_lock_task(t);
    {
      ret = t->m_status;
      t->m_status = s;
    }
    s_unlock_task(t);

  }

  return ret;
}





static lagopus_result_t
s_runnable_run(const lagopus_runnable_t *rptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)arg;

  if (likely(rptr != NULL && *rptr != NULL)) {
    lagopus_callout_task_t t = (lagopus_callout_task_t)*rptr;
    if (likely(t->m_proc != NULL)) {
      ret = (t->m_proc)(t->m_arg);
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_create_task(lagopus_callout_task_t *tptr,
              size_t sz,
              callout_task_type_t type,
              const char *name,
              lagopus_callout_task_proc_t proc,
              void *arg,
              lagopus_callout_task_arg_freeup_proc_t freeproc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *nm = NULL;

  if (IS_VALID_STRING(name) == true) {
    if (unlikely((nm = strdup(name)) == NULL)) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      goto done;
    }
  }

  if (sz == 0) {
    sz = DEFAULT_TASK_ALLOC_SZ;
  }

  if (unlikely((ret = lagopus_mutex_create_recursive(&((*tptr)->m_lock))) == 
               LAGOPUS_RESULT_OK)) {
    goto done;
  }

  s_lock_global();
  {
    if (likely((ret = lagopus_runnable_create((lagopus_runnable_t *)tptr, sz,
                                              s_runnable_run, NULL, NULL)) ==
               LAGOPUS_RESULT_OK)) {

      (void)memset((void *)&((*tptr)->m_entry), 0, sizeof((*tptr)->m_entry));

      (*tptr)->m_status = TASK_STATE_CREATED;
      (*tptr)->m_exec_ref_count = 0;
      (*tptr)->m_type = type;
      (*tptr)->m_name = nm;
      (*tptr)->m_proc = proc;
      (*tptr)->m_arg = arg;
      (*tptr)->m_freeproc = freeproc;
      (*tptr)->m_do_repeat = false;
      (*tptr)->m_is_first = true;
      (*tptr)->m_is_in_timed_q = false;
      (*tptr)->m_initial_delay_time = -1LL;
      (*tptr)->m_interval_time = -1LL;
      (*tptr)->m_last_abstime = 0;
      (*tptr)->m_next_abstime = 0;

      if (unlikely(s_set_task_state_in_table(*tptr, TASK_STATE_CREATED) ==
                   TASK_STATE_CREATED)) {
        /*
         * Avoid arg be freed by destroyer at this moment.
         */
        (*tptr)->m_arg = NULL;
        (*tptr)->m_freeproc = NULL;
        s_destroy_task_no_lock(*tptr);
        nm = NULL;
      }
    }
  }
  s_unlock_global();

done:
  if (ret != LAGOPUS_RESULT_OK && nm != NULL) {
    free((void *)nm);
  }

  return ret;
}





static inline void
s_delete(lagopus_callout_task_t t) {
  lagopus_runnable_t r = (lagopus_runnable_t)t;
  callout_task_state_t st = s_get_task_state_in_table(t);

  if ((st == TASK_STATE_UNKNOWN || 
       st == TASK_STATE_CANCELLED) &&
      __sync_fetch_and_add(&(t->m_exec_ref_count), 0) == 0) {
    bool did_delete = false;

    s_lock_task(t);
    {
      if (likely(t->m_status != TASK_STATE_DELETING)) {
        t->m_status = TASK_STATE_DELETING;
        did_delete = true;

        if (t->m_freeproc != NULL && t->m_arg != NULL) {
          (t->m_freeproc)(t->m_arg);
        }

        if (IS_VALID_STRING(t->m_name) == true) {
          free((void *)t->m_name);
        }
      }
    }
    s_unlock_task(t);

    if (likely(did_delete == true)) {
      lagopus_mutex_destroy(&(t->m_lock));

      s_delete_task_in_table(t);
      lagopus_runnable_destroy(&r);
    }
  }
}


static inline void
s_destroy_task_no_lock(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    s_delete(t);
  }
}


static inline void
s_destroy_task(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {

    s_lock_global();
    {
      s_delete(t);
    }
    s_unlock_global();

  }
}





static inline void
s_cancel(const lagopus_callout_task_t t) {
  callout_task_state_t ostate = 
      s_set_task_state_in_table(t, TASK_STATE_CANCELLED);
  if (ostate == TASK_STATE_ENQUEUED) {
    s_unschedule_timed_task_no_lock(t);
    s_destroy_task_no_lock(t);
  }
}


static inline void
s_cancel_task_no_lock(const lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    s_cancel(t);
  }
}


static inline void
s_cancel_task(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {

    s_lock_global();
    {
      s_cancel(t);
    }
    s_unlock_global();

  }
}





static inline lagopus_result_t
s_exec_task(lagopus_callout_task_t t) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(t != NULL)) {
    callout_task_state_t st = s_get_task_state_in_table(t);
    bool do_delete = false;
    lagopus_runnable_t r = (lagopus_runnable_t)t;
    size_t is_execing = 0;

    if (likely(st == TASK_STATE_DEQUEUED)) {

      is_execing = __sync_fetch_and_add(&(t->m_exec_ref_count), 1);
      if (likely(is_execing == 0)) {

        s_lock_task(t);
        {
          if (likely(t->m_status == TASK_STATE_DEQUEUED)) {
            /*
             * Change the state to TASK_STATE_EXECUTING.
             */
            (void)s_set_task_state_in_table(t, TASK_STATE_EXECUTING);
            t->m_status = TASK_STATE_EXECUTING;

            /*
             * Then execute the task.
             */
            t->m_is_first = false;
            ret = lagopus_runnable_start(&r);
            if (likely(ret == LAGOPUS_RESULT_OK)) {
              /*
               * The execution succeeded.
               */
              if (t->m_do_repeat == true) {
                ret = s_schedule_timed_task_no_lock(t);
                if (likely(ret >= 0)) {
                  ret = LAGOPUS_RESULT_OK;
                } else {
                  /*
                   * Make this task to be deleted.
                   */
                  do_delete = true;
                  lagopus_perror(ret);
                  lagopus_msg_error_with_task(t,
                                              "can't re-schedule the task.\n");
                }
              } else {
                do_delete = true;
              }
            } else {
              /*
               * The execution failed.
               */
              s_set_task_state_in_table(t, TASK_STATE_EXEC_FAILED);
              t->m_status = TASK_STATE_EXEC_FAILED;
              lagopus_perror(ret);
              lagopus_msg_error_with_task(t,
                                          "task execution failure.\n");
              do_delete = true;
            }
          }
        }
        s_unlock_task(t);

        if (do_delete == true || ret != LAGOPUS_RESULT_OK) {
          /*
           * Delete the task. Don't change the return value anymore.
           */

          s_lock_global();
          {
            s_delete_task_in_table(t);
            s_destroy_task_no_lock(t);
          }
          s_unlock_global();

        }

      } else {	/* is_execing == 0 */
        /*
         * Do really nothing.
         */
        ret = LAGOPUS_RESULT_OK;
      }

      (void)__sync_fetch_and_sub(&(t->m_exec_ref_count), 1);

    } else {	/* st == TASK_STATE_DEQUEUED */

      /*
       * Illegal state, the most likely already executing.
       */

      if (likely(st == TASK_STATE_EXECUTING)) {
        /*
         * Do nothing.
         */
        ret = LAGOPUS_RESULT_OK;
      } else if (st == TASK_STATE_UNKNOWN ||
                 st == TASK_STATE_CANCELLED) {
        /*
         * Deleted.
         */

        s_lock_global();
        {
          s_delete_task_in_table(t);
          s_destroy_task_no_lock(t);
        }
        s_unlock_global();

        /*
         * And make the return value a "not OK".
         */
        ret = LAGOPUS_RESULT_INVALID_OBJECT;
      } else {
        ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
      }
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_submit_task(lagopus_callout_task_t t,
              lagopus_chrono_t initial_delay,
              lagopus_chrono_t interval) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(t != NULL)) {

    s_lock_task(t);
    {
      t->m_initial_delay_time = initial_delay;
      if (interval > 0LL) {
        t->m_interval_time = interval;
        t->m_do_repeat = true;
      }

      if (likely(initial_delay == 0LL)) {
        /*
         * An urgent task.
         */
        ret = lagopus_bbq_put(&s_urgent_tsk_q, (void **)t,
                              lagopus_callout_task_t, -1LL);
      } else if (likely(initial_delay > 0LL)) {
        /*
         * A timed task.
         */
        ret = s_schedule_timed_task(t);
        if (likely(ret == LAGOPUS_RESULT_OK &&
                   __sync_fetch_and_add(&s_next_wakeup_abstime, 0) 
                   > initial_delay)) {
          ret = lagopus_bbq_wakeup(&s_urgent_tsk_q, -1LL);
        }
      } else {
        /*
         * An idle task.
         */
        ret = lagopus_bbq_put(&s_idle_tsk_q, (void **)t,
                              lagopus_callout_task_t, -1LL);
        goto unlock_task;
      }

      if (ret == LAGOPUS_RESULT_OK && t->m_is_in_timed_q == false) {
        /*
         * For the tasks except the timed ones, set the task state to
         * TASK_STATE_ENQUEUED.
         */
        (void)s_set_task_state_in_table(t, TASK_STATE_ENQUEUED);
        t->m_status = TASK_STATE_ENQUEUED;
      }
    }
 unlock_task:
    s_unlock_task(t);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_force_schedule_task(lagopus_callout_task_t t) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(t != NULL)) {
    lagopus_callout_task_t ta[1] = { t };

    s_lock_task(t);
    {
      s_unschedule_timed_task_no_lock(t);
      /*
       * Set the task state to TASK_STATE_DEQUEUED.
       */
      (void)s_set_task_state_in_table(t, TASK_STATE_DEQUEUED);
      t->m_status = TASK_STATE_DEQUEUED;

      ret = s_submit_callout_stage(ta, 1);
    }
    s_unlock_task(t);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
