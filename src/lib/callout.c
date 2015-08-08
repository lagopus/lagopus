/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include "lagopus_apis.h"
#include "lagopus_callout_internal.h"
#include "lagopus_pipeline_stage_internal.h"





#define DEFAULT_TASK_ALLOC_SZ	(sizeof(lagopus_callout_task_record))

#define CALLOUT_TASK_MAX	1000


#define lagopus_msg_error_with_task(t, str, ...) {                      \
    do {                                                                \
      if (IS_VALID_STRING((t)->m_name) == true) {                       \
        lagopus_msg_error("%s:" str, (t)->m_name, ##__VA_ARGS__);       \
      } else {                                                          \
        lagopus_msg_error("%p:" str, (void *)(t), ##__VA_ARGS__);       \
      }                                                                 \
    } while (0);                                                        \
  }





TAILQ_HEAD(chrono_task_queue_t, lagopus_callout_task_record);
typedef struct chrono_task_queue_t chrono_task_queue_t;





static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static lagopus_mutex_t s_lck = NULL;		/* The global lock. */
static lagopus_cond_t s_cnd = NULL;		/* The cond. */

static lagopus_hashmap_t s_tsk_tbl = NULL;	/* The valid tasks table. */

static lagopus_bbq_t s_urgent_tsk_q = NULL;	/* The urgent tasks Q. */
static chrono_task_queue_t s_chrono_tsk_q;	/* The timed tasks Q. */
static lagopus_mutex_t s_q_lck;			/* The timed task Q lock. */
static lagopus_bbq_t s_idle_tsk_q = NULL;	/* The Idle taskss Q. */

static lagopus_callout_idle_proc_t s_idle_proc = NULL;
static lagopus_callout_idle_arg_freeup_proc_t s_free_proc = NULL;
static void *s_idle_proc_arg = NULL;





static void s_ctors(void) __attr_constructor__(106);
static void s_dtors(void) __attr_destructor__(106);


static void s_lock_global(void);
static void s_unlock_global(void);
static bool s_is_global_locked(void);

static void s_lock_task(lagopus_callout_task_t t);
static void s_unlock_task(lagopus_callout_task_t t);
static bool s_is_task_locked(lagopus_callout_task_t t);

static void s_task_freeup(void **valptr);

/**
 * _no_lock() version of the APIs need to be between
 * s_lock_global()/s_unlock_global()
 */
static callout_task_state_t s_set_task_state_no_lock(lagopus_callout_task_t t,
                                                     callout_task_state_t s);
static callout_task_state_t s_set_task_state(lagopus_callout_task_t t,
                                             callout_task_state_t s);
static callout_task_state_t
s_get_task_state_no_lock(const lagopus_callout_task_t t);
static callout_task_state_t
s_get_task_state(const lagopus_callout_task_t t);
static void s_delete_task_no_lock(const lagopus_callout_task_t t);
static void s_delete_task(const lagopus_callout_task_t t);

/**
 * The timed task scheduler/unscheduler need to be in
 * s_lock_task()/s_unlock_task().
 */
static lagopus_result_t
s_schedule_timed_task_no_lock(lagopus_callout_task_t t);
static lagopus_result_t s_schedule_timed_task(lagopus_callout_task_t t);
static void s_unschedule_timed_task_no_lock(lagopus_callout_task_t t);
static void s_unschedule_timed_task(lagopus_callout_task_t t);

static lagopus_result_t
s_create_task(lagopus_callout_task_t *tptr,
              size_t sz,
              callout_task_type_t type,
              const char *name,
              lagopus_callout_task_proc_t proc,
              lagopus_callout_task_get_event_n_proc_t get_n_proc,
              void *arg,
              lagopus_callout_task_arg_freeup_proc_t freeproc);
static void s_destroy_task(lagopus_callout_task_t);
static void s_destroy_task_no_lock(lagopus_callout_task_t);
static lagopus_result_t s_exec_task(lagopus_callout_task_t t);





#include "callout_stage.c"
#include "callout_queue.c"





static void
s_task_freeup(void **valptr) {
  if (likely(valptr != NULL && *valptr != NULL)) {
    /*
     * It should be unnecessary to unschedule the task but for in
     * case.
     */
    s_unschedule_timed_task((lagopus_callout_task_t)*valptr);
    s_delete_task((lagopus_callout_task_t)*valptr);
    s_destroy_task((lagopus_callout_task_t)*valptr);
  }
}


static void
s_once_proc(void) {
  lagopus_result_t r;

  if ((r = lagopus_mutex_create(&s_lck)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout global mutex.\n");
  }

  if ((r = lagopus_cond_create(&s_cnd)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout cond.\n");
  }

  if ((r = lagopus_hashmap_create(&s_tsk_tbl,
                                  LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                  NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout table.\n");
  }

  if ((r = lagopus_bbq_create(&s_urgent_tsk_q, lagopus_callout_task_t,
                              CALLOUT_TASK_MAX, s_task_freeup)) != 
      LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout urgent tasks queue.\n");
  }

  if ((r = lagopus_bbq_create(&s_idle_tsk_q, lagopus_callout_task_t,
                              CALLOUT_TASK_MAX, s_task_freeup)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout idle tasks queue.\n");
  }

  if ((r = lagopus_mutex_create(&s_q_lck)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout task queue mutex.\n");
  }

  TAILQ_INIT(&s_chrono_tsk_q);
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(10, "The callout module is initialized.\n");
}


static inline void
s_final(void) {
  lagopus_bbq_destroy(&s_idle_tsk_q, true);
  lagopus_bbq_destroy(&s_urgent_tsk_q, true);
  lagopus_hashmap_destroy(&s_tsk_tbl, true);
  lagopus_cond_destroy(&s_cnd);
  lagopus_mutex_destroy(&s_lck);
}


static void
s_dtors(void) {
  s_final();

  lagopus_msg_debug(10, "The callout module is finalized.\n");
}





static inline void
s_lock_global(void) {
  if (likely(s_lck != NULL)) {
    (void)lagopus_mutex_lock(&s_lck);
  }
}


static inline void
s_unlock_global(void) {
  if (likely(s_lck != NULL)) {
    (void)lagopus_mutex_unlock(&s_lck);
  }
}


static inline bool
s_is_global_locked(void) {
  lagopus_result_t r = lagopus_mutex_trylock(&s_lck);

  if (r == LAGOPUS_RESULT_OK) {
    (void)lagopus_mutex_unlock(&s_lck);
    return true;
  } else {
    return false;
  }
}


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


static inline bool
s_is_task_locked(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    lagopus_result_t r = lagopus_mutex_trylock(&(t->m_lock));

    if (r == LAGOPUS_RESULT_OK) {
      (void)lagopus_mutex_unlock(&(t->m_lock));
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}





static inline callout_task_state_t
s_get_task_state_no_lock(const lagopus_callout_task_t t) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    lagopus_result_t r;

    r = lagopus_hashmap_find_no_lock(&s_tsk_tbl, (void *)t, &val);
    if (likely(r == LAGOPUS_RESULT_OK)) {
      ret = (callout_task_state_t)val;
    }
  }

  return ret;
}


static inline callout_task_state_t
s_get_task_state(const lagopus_callout_task_t t) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    lagopus_result_t r;

    s_lock_global();
    {
      r = lagopus_hashmap_find_no_lock(&s_tsk_tbl, (void *)t, &val);
    }
    s_unlock_global();

    if (likely(r == LAGOPUS_RESULT_OK)) {
      ret = (callout_task_state_t)val;
    }
  }

  return ret;
}


static inline callout_task_state_t
s_set_task_state_no_lock(const lagopus_callout_task_t t,
                         callout_task_state_t s) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    lagopus_result_t r;

    if (likely((r = lagopus_hashmap_find_no_lock(&s_tsk_tbl,
                                                 (void *)t, &val)) ==
               LAGOPUS_RESULT_OK)) {
      ret = (callout_task_state_t)val;
      val = (void *)s;

      if (unlikely((r != lagopus_hashmap_add_no_lock(&s_tsk_tbl, (void *)t,
                                                     &val, true)) ==
                   LAGOPUS_RESULT_OK)) {
        ret = TASK_STATE_UNKNOWN;
      }
    } else if (likely(r == LAGOPUS_RESULT_NOT_FOUND)) {
      val = (void *)s;
      if (likely((r = lagopus_hashmap_add_no_lock(&s_tsk_tbl, (void *)t,
                                                  &val, true)) ==
                 LAGOPUS_RESULT_OK)) {
        ret = s;
      }
    }
  }

  return ret;
}


static inline callout_task_state_t
s_set_task_state(const lagopus_callout_task_t t,
                 callout_task_state_t s) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    lagopus_result_t r;

    s_lock_global();
    {
      if (likely((r = lagopus_hashmap_find_no_lock(&s_tsk_tbl,
                                                   (void *)t, &val)) ==
                 LAGOPUS_RESULT_OK)) {
        ret = (callout_task_state_t)val;
        val = (void *)s;

        if (unlikely((r != lagopus_hashmap_add_no_lock(&s_tsk_tbl, (void *)t,
                                                       &val, true)) ==
                     LAGOPUS_RESULT_OK)) {
          ret = TASK_STATE_UNKNOWN;
        }
      } else if (likely(r == LAGOPUS_RESULT_NOT_FOUND)) {
        val = (void *)s;
        if (likely((r = lagopus_hashmap_add_no_lock(&s_tsk_tbl, (void *)t,
                                                    &val, true)) ==
                   LAGOPUS_RESULT_OK)) {
          ret = s;
        }
      }
    }
    s_unlock_global();

  }

  return ret;
}


static inline void
s_delete_task_no_lock(const lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)lagopus_hashmap_delete_no_lock(&s_tsk_tbl, (void *)t, NULL, true);
  }
}


static inline void
s_delete_task(const lagopus_callout_task_t t) {
  if (likely(t != NULL)) {

    s_lock_global();
    {
      (void)lagopus_hashmap_delete_no_lock(&s_tsk_tbl, (void *)t, NULL, true);
    }
    s_unlock_global();

  }
}



/*
 * Runnable
 */


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
              lagopus_callout_task_get_event_n_proc_t get_n_proc,
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

  if (unlikely((ret = lagopus_mutex_create(&((*tptr)->m_lock))) == 
               LAGOPUS_RESULT_OK)) {
    goto done;
  }

  s_lock_global();
  {
    if (likely((ret = lagopus_runnable_create((lagopus_runnable_t *)tptr, sz,
                                              s_runnable_run, NULL, NULL)) ==
               LAGOPUS_RESULT_OK)) {

      (void)memset((void *)&((*tptr)->m_entry), 0, sizeof((*tptr)->m_entry));

      (*tptr)->m_type = type;
      (*tptr)->m_name = nm;
      (*tptr)->m_proc = proc;
      (*tptr)->m_get_n_proc = get_n_proc;
      (*tptr)->m_arg = arg;
      (*tptr)->m_freeproc = freeproc;
      (*tptr)->m_do_repeat = false;
      (*tptr)->m_is_first = true;
      (*tptr)->m_initial_delay_time = -1LL;
      (*tptr)->m_interval_time = -1LL;
      (*tptr)->m_last_abstime = 0;
      (*tptr)->m_next_abstime = 0;

      if (unlikely(s_set_task_state_no_lock(*tptr, TASK_STATE_CREATED) ==
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
s_destroy_task_no_lock(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    lagopus_runnable_t r = (lagopus_runnable_t)t;
    callout_task_state_t st;

    st = s_get_task_state_no_lock(t);
    if (st == TASK_STATE_UNKNOWN || st == TASK_STATE_CANCELLED) {

      if (t->m_freeproc != NULL && t->m_arg != NULL) {
        (t->m_freeproc)(t->m_arg);
      }

      if (IS_VALID_STRING(t->m_name) == true) {
        free((void *)t->m_name);
      }

      lagopus_mutex_destroy(&(t->m_lock));

      s_delete_task_no_lock(t);
      lagopus_runnable_destroy(&r);
    }
  }
}


static inline void
s_destroy_task(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    lagopus_runnable_t r = (lagopus_runnable_t)t;
    callout_task_state_t st;

    s_lock_global();
    {
      st = s_get_task_state_no_lock(t);
      if (st == TASK_STATE_UNKNOWN || st == TASK_STATE_CANCELLED) {

        if (t->m_freeproc != NULL && t->m_arg != NULL) {
          (t->m_freeproc)(t->m_arg);
        }

        if (IS_VALID_STRING(t->m_name) == true) {
          free((void *)t->m_name);
        }

        lagopus_mutex_destroy(&(t->m_lock));

        s_delete_task_no_lock(t);
        lagopus_runnable_destroy(&r);
      }
    }
    s_unlock_global();

  }
}


static inline void
s_cancel_task_no_lock(const lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    callout_task_state_t ostate;

    ostate = s_set_task_state_no_lock(t, TASK_STATE_CANCELLED);
    if (ostate == TASK_STATE_ENQUEUED) {
      s_unschedule_timed_task_no_lock(t);
      s_destroy_task_no_lock(t);
    }
  }
}


static inline void
s_cancel_task(lagopus_callout_task_t t) {
  if (likely(t != NULL)) {
    callout_task_state_t ostate;

    s_lock_global();
    {
      ostate = s_set_task_state_no_lock(t, TASK_STATE_CANCELLED);
      if (ostate == TASK_STATE_ENQUEUED) {
        s_unschedule_timed_task_no_lock(t);
        s_destroy_task_no_lock(t);
      }
    }
    s_unlock_global();

  }
}


static inline lagopus_result_t
s_exec_task(lagopus_callout_task_t t) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(t != NULL)) {
    callout_task_state_t st;
    bool do_delete = false;
    bool do_resched = false;
    lagopus_runnable_t r = (lagopus_runnable_t)t;

    /*
     * At this moment here could be some other threads trying to
     * execute this task. Block those threads by taking the global
     * lock first.
     */

    s_lock_global();
    {

      /*
       * Then check the curent state of the task and go on if the
       * state is the TASK_STATE_DEQUEUED.
       */
      st = s_get_task_state_no_lock(t);
      if (likely(st == TASK_STATE_DEQUEUED)) {

        /*
         * Change the state to TASK_STATE_EXECUTING.
         */
        (void)s_set_task_state_no_lock(t, TASK_STATE_EXECUTING);

        s_lock_task(t);
        {
          /*
           * Note we still have the global lock. While we have the
           * global lock, we also acquire the task lock.
           */

          s_unlock_global();
          /*
           * Now the global lock is released.
           */

          /*
           * Note that the any other threads trying to execute this
           * task at this moment now know that the task state is the
           * TASK_STATE_EXECUTING and just leave.
           */

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
              do_resched = true;
            }
          } else {
            /*
             * The execution failed.
             */
            s_set_task_state(t, TASK_STATE_EXEC_FAILED);
            lagopus_perror(ret);
            lagopus_msg_error_with_task(t,
                                        "task execution failure.\n");
            do_delete = true;
          }

          if (do_resched == true || do_delete == true) {

            s_lock_global();

            /*
             * Now the global lock is acquired again to be safe about
             * the task deletion or re-scheduling.
             */
            if (do_resched == true) {
              /*
               * Re-schedule this task.
               */
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
            }

          } else {
            goto final_global_unlock;
          }

        }
        s_unlock_task(t);

        /*
         * We still/again have the global lock. Use "no lock" version
         * of the APIs.
         */
        if (do_delete == true) {
          /*
           * Delete the task.
           */
          s_delete_task_no_lock(t);
          s_destroy_task_no_lock(t);
          /*
           * Note that the return value is determined at this
           * moment. Don't change it.
           */
        }

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
          s_delete_task_no_lock(t);
          s_destroy_task_no_lock(t);

          /*
           * And make the return value a "not OK".
           */
          ret = LAGOPUS_RESULT_INVALID_OBJECT;
        } else {
          ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
        }
      }

    }
 final_global_unlock:
    s_unlock_global();

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





