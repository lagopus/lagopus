/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */





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
s_lock_task_q(void) {
  if (likely(s_q_lck != NULL)) {
    (void)lagopus_mutex_lock(&s_q_lck);
  }
}


static inline void
s_unlock_task_q(void) {
  if (likely(s_q_lck != NULL)) {
    (void)lagopus_mutex_unlock(&s_q_lck);
  }
}





static inline lagopus_chrono_t
s_do_sched(lagopus_callout_task_t t) {
  lagopus_result_t ret = -1LL;
  lagopus_result_t r;
  lagopus_callout_task_t e;

  s_lock_task_q();
  {

    s_lock_task(t);
    {
      if (likely(t->m_is_in_timed_q == false)) {

        /*
         * Firstly (re-)compute the next exeution time of this
         * task.
         */
        if (t->m_is_first == false && t->m_do_repeat == true) {
          t->m_next_abstime = t->m_last_abstime + t->m_interval_time;
        } else {
          WHAT_TIME_IS_IT_NOW_IN_NSEC(t->m_last_abstime);
          t->m_next_abstime = t->m_last_abstime + t->m_initial_delay_time;
        }

        /*
         * Then insert the task into the Q.
         */
        e = TAILQ_FIRST(&s_chrono_tsk_q);
        if (e != NULL) {
          if (e->m_next_abstime > t->m_next_abstime) {
            TAILQ_INSERT_HEAD(&s_chrono_tsk_q, t, m_entry);
          } else {
            for (;
                 e != NULL && e->m_next_abstime <= t->m_next_abstime;
                 e = TAILQ_NEXT(e, m_entry)) {
              ;
            }
            if (e != NULL) {
              TAILQ_INSERT_BEFORE(e, t, m_entry);
            } else {
              TAILQ_INSERT_TAIL(&s_chrono_tsk_q, t, m_entry);
            }
          }
        } else {
          TAILQ_INSERT_TAIL(&s_chrono_tsk_q, t, m_entry);
        }

        (void)s_set_task_state_in_table(t, TASK_STATE_ENQUEUED);
        t->m_status = TASK_STATE_ENQUEUED;
        t->m_is_in_timed_q = true;

        ret = t->m_next_abstime;

        /*
         * Fianlly wake the master scheduler.
         */
        lagopus_msg_debug(4, "wake the master scheduler up.\n");

        /*
         * TODO
         *
         *	Re-optimize forcible wake up by timed task submission
         *	timing and times. See also
         *	callout.c:s_start_callout_main_loop()
         */

        r = lagopus_bbq_wakeup(&s_urgent_tsk_q, 0LL);
        if (unlikely(r != LAGOPUS_RESULT_OK)) {
          lagopus_perror(r);
          lagopus_msg_error("can't wake the callout task master "
                            "scheduler up.\n");
        }
      }
    }
    s_unlock_task(t);

  }
  s_unlock_task_q();

  return ret;
}


static inline lagopus_chrono_t
s_schedule_timed_task_no_lock(lagopus_callout_task_t t) {
  lagopus_result_t ret = -1LL;

  if (likely(t != NULL)) {
    ret = s_do_sched(t);
  }

  return ret;
}


static inline lagopus_chrono_t
s_schedule_timed_task(lagopus_callout_task_t t) {
  lagopus_result_t ret = -1LL;

  if (likely(t != NULL)) {

    s_lock_global();
    {
      ret = s_do_sched(t);
    }
    s_unlock_global();

  }

  return ret;
}





static inline void
s_do_unsched(lagopus_callout_task_t t) {

  s_lock_task_q();
  {

    s_lock_task(t);
    {
      if (likely(t->m_is_in_timed_q == true)) {

        TAILQ_REMOVE(&s_chrono_tsk_q, t, m_entry);

        if (t->m_status == TASK_STATE_ENQUEUED) {
            (void)s_set_task_state_in_table(t, TASK_STATE_DEQUEUED);
            t->m_status = TASK_STATE_DEQUEUED;
        }

        t->m_is_in_timed_q = false;
      }
    }
    s_unlock_task(t);
      
  }
  s_unlock_task_q();

}


static inline void
s_unschedule_timed_task_no_lock(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    s_do_unsched(t);
  }
}


static inline void
s_unschedule_timed_task(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {

    s_lock_global();
    {
      s_do_unsched(t);
    }
    s_unlock_global();

  }
}





static inline lagopus_callout_task_t
s_peek_timed_task(void) {
  lagopus_callout_task_t ret = NULL;

  s_lock_task_q();
  {
    ret = TAILQ_FIRST(&s_chrono_tsk_q);
  }
  s_unlock_task_q();

  return ret;
}


static inline lagopus_chrono_t
s_peek_current_wakeup_time(void) {
  lagopus_chrono_t ret = -1LL;
  lagopus_callout_task_t t;

  s_lock_task_q();
  {
    t = TAILQ_FIRST(&s_chrono_tsk_q);
    if (t != NULL) {

      s_lock_task(t);
      {
        ret = t->m_next_abstime;
      }
      s_unlock_task(t);

    }
  }
  s_unlock_task_q();

  return ret;
}





static inline lagopus_callout_task_t
s_get(void) {
  lagopus_callout_task_t ret = NULL;

  s_lock_task_q();
  {
    ret = TAILQ_FIRST(&s_chrono_tsk_q);

    if (likely(ret != NULL)) {

      s_lock_task(ret);
      {
        TAILQ_REMOVE(&s_chrono_tsk_q, ret, m_entry);
        (void)s_set_task_state_in_table(ret, TASK_STATE_DEQUEUED);
        ret->m_status = TASK_STATE_DEQUEUED;
        ret->m_is_in_timed_q = false;
      }
      s_unlock_task(ret);

    }
  }
  s_unlock_task_q();

  return ret;
}


static inline lagopus_callout_task_t
s_get_timed_task_no_lock(void) {
  return s_get();
}


static inline lagopus_callout_task_t
s_get_timed_task(void) {
  lagopus_callout_task_t ret = NULL;

  s_lock_global();
  {
    ret = s_get();
  }
  s_unlock_global();

  return ret;
}





static inline lagopus_result_t
s_get_runnables(lagopus_chrono_t base_abstime,
                lagopus_callout_task_t *tasks, size_t n,
                lagopus_chrono_t *next_wakeup) {
  size_t n_ret = 0LL;
  lagopus_callout_task_t e;
  lagopus_chrono_t the_abstime = base_abstime + CALLOUT_TASK_SCHED_JITTER;

  s_lock_task_q();
  {

    e = TAILQ_FIRST(&s_chrono_tsk_q);

    while (n_ret < n && e != NULL) {
      e = TAILQ_FIRST(&s_chrono_tsk_q);
      if (likely(e != NULL &&
                 e->m_next_abstime <= the_abstime)) {
        TAILQ_REMOVE(&s_chrono_tsk_q, e, m_entry);

        s_lock_task(e);
        {
          (void)s_set_task_state_in_table(e, TASK_STATE_DEQUEUED);
          e->m_status = TASK_STATE_DEQUEUED;
          e->m_is_in_timed_q = false;
        }
        s_unlock_task(e);

        tasks[n_ret++] = e;
      } else {
        break;
      }
    }

    if (likely(next_wakeup != NULL)) {
      e = TAILQ_FIRST(&s_chrono_tsk_q);
      if (e != NULL) {
        *next_wakeup = e->m_next_abstime;
      } else {
        *next_wakeup = -1LL;
      }
    }

  }
  s_unlock_task_q();

  return (lagopus_result_t)n_ret;
}


static inline lagopus_result_t
s_get_runnable_timed_task_no_lock(lagopus_chrono_t base_abstime,
                                  lagopus_callout_task_t *tasks, size_t n,
                                  lagopus_chrono_t *next_wakeup) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(base_abstime > 0LL &&
             tasks != NULL && n > 0)) {
    ret = s_get_runnables(base_abstime, tasks, n, next_wakeup);
  }

  return ret;
}
             

static inline lagopus_result_t
s_get_runnable_timed_task(lagopus_chrono_t base_abstime,
                          lagopus_callout_task_t *tasks, size_t n,
                          lagopus_chrono_t *next_wakeup) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(base_abstime > 0LL &&
             tasks != NULL && n > 0)) {

    s_lock_global();
    {
      ret = s_get_runnables(base_abstime, tasks, n, next_wakeup);
    }
    s_unlock_global();

  }

  return ret;
}
             

static inline void
s_destroy_all_queued_timed_tasks(void) {
  lagopus_callout_task_t t;

  s_lock_global();
  {

    while ((t = s_get_timed_task_no_lock()) != NULL) {
      s_set_cancel_and_destroy_task_no_lock(t);
    }

  }
  s_unlock_global();
}

