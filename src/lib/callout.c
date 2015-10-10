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

#define CALLOUT_TASK_SCHED_JITTER	1000LL	/* 1 usec. */

#define CALLOUT_IDLE_PROC_MIN_INTERVAL	1000LL * 1000LL /* 1 msec. */

#define CALLOUT_STAGE_SHUTDOWN_TIMEOUT	5LL * 1000LL * 1000LL * 1000LL
					/* 5 sec. */

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


typedef lagopus_result_t
(*final_task_schedule_proc_t)(const lagopus_callout_task_t * const tasks,
                                 size_t n);





static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static lagopus_mutex_t s_sched_lck = NULL;	/* The scheduler main lock. */
static lagopus_cond_t s_sched_cnd = NULL;		/* The scheduler cond. */
static lagopus_mutex_t s_lck = NULL;		/* The global lock. */

static lagopus_hashmap_t s_tsk_tbl = NULL;	/* The valid tasks table. */

static lagopus_bbq_t s_urgent_tsk_q = NULL;	/* The urgent tasks Q. */
static chrono_task_queue_t s_chrono_tsk_q;	/* The timed tasks Q. */
static lagopus_mutex_t s_q_lck;			/* The timed task Q lock. */
static lagopus_bbq_t s_idle_tsk_q = NULL;	/* The Idle taskss Q. */

static lagopus_callout_idle_proc_t s_idle_proc = NULL;
static lagopus_callout_idle_arg_freeup_proc_t s_free_proc = NULL;
static void *s_idle_proc_arg = NULL;
static lagopus_chrono_t s_idle_interval = -1LL;
static lagopus_chrono_t s_next_idle_abstime = -1LL;

static size_t s_n_workers;
static volatile bool s_do_loop = false;		/* The main loop on/off */
static volatile bool s_is_stopped = false;	/* The main loop is
                                                 * stopped or not. */

static volatile lagopus_chrono_t s_next_wakeup_abstime;

static volatile bool s_is_handler_inited = false;


static final_task_schedule_proc_t s_final_task_sched_proc = NULL;





static void s_ctors(void) __attr_constructor__(106);
static void s_dtors(void) __attr_destructor__(106);


/**
 * General lock order:
 *
 *	The global lock (s_lock_global)
 *	The timed task queue lock (s_lock_task_q)
 *	A task lock (s_lock_task)
 *
 * Note taht the task locks are recursive.
 */


static void s_lock_global(void);
static void s_unlock_global(void);

static void s_lock_task_q(void);
static void s_unlock_task_q(void);

static void s_lock_task(lagopus_callout_task_t t);
static void s_unlock_task(lagopus_callout_task_t t);

static void s_wait_task(lagopus_callout_task_t t);
static void s_wakeup_task(lagopus_callout_task_t t);

static callout_task_state_t s_set_task_state_in_table(
    const lagopus_callout_task_t t,
    callout_task_state_t s);
static callout_task_state_t s_get_task_state_in_table(
    const lagopus_callout_task_t t);
static inline void s_delete_task_in_table(const lagopus_callout_task_t t);

static callout_task_state_t s_set_task_state(lagopus_callout_task_t t,
                                             callout_task_state_t s);
static callout_task_state_t s_get_task_state(const lagopus_callout_task_t t);


/**
 * _no_lock() version of the APIs need to be between
 * s_lock_global()/s_unlock_global()
 */
static lagopus_result_t
s_create_task(lagopus_callout_task_t *tptr,
              size_t sz,
              const char *name,
              lagopus_callout_task_proc_t proc,
              void *arg,
              lagopus_callout_task_arg_freeup_proc_t freeproc);

static void s_destroy_task_no_lock(lagopus_callout_task_t);
static void s_destroy_task(lagopus_callout_task_t);

static void s_set_cancel_and_destroy_task_no_lock(lagopus_callout_task_t t);
static void s_set_cancel_and_destroy_task(lagopus_callout_task_t t);

static inline void s_cancel_task_no_lock(const lagopus_callout_task_t t);
static inline void s_cancel_task(const lagopus_callout_task_t t);

static lagopus_result_t s_schedule_timed_task_no_lock(
    lagopus_callout_task_t t);
static lagopus_result_t s_schedule_timed_task(lagopus_callout_task_t t);
static void s_unschedule_timed_task_no_lock(lagopus_callout_task_t t);
static void s_unschedule_timed_task(lagopus_callout_task_t t);

static lagopus_result_t s_exec_task(lagopus_callout_task_t t);

static lagopus_result_t s_submit_callout_stage(
    const lagopus_callout_task_t * const tasks, size_t n);

static void s_task_freeup(void **valptr);





#include "callout_task.c"
#include "callout_stage.c"
#include "callout_queue.c"





static void
s_task_freeup(void **valptr) {
  if (likely(valptr != NULL && *valptr != NULL)) {

    s_lock_global();
    {
      s_set_cancel_and_destroy_task_no_lock((lagopus_callout_task_t)*valptr);
    }
    s_unlock_global();

  }
}


static void
s_once_proc(void) {
  lagopus_result_t r;

  if ((r = lagopus_mutex_create(&s_sched_lck)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout scheduler main mutex.\n");
  }

  if ((r = lagopus_mutex_create(&s_lck)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the callout global mutex.\n");
  }

  if ((r = lagopus_cond_create(&s_sched_cnd)) != LAGOPUS_RESULT_OK) {
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

  if ((r = lagopus_mutex_create_recursive(&s_q_lck)) != LAGOPUS_RESULT_OK) {
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
  lagopus_cond_destroy(&s_sched_cnd);
  lagopus_mutex_destroy(&s_lck);
  lagopus_mutex_destroy(&s_sched_lck);
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


static inline void
s_lock_sched(void) {
  if (likely(s_sched_lck != NULL)) {
    (void)lagopus_mutex_lock(&s_sched_lck);
  }
}


static inline void
s_unlock_sched(void) {
  if (likely(s_sched_lck != NULL)) {
    (void)lagopus_mutex_unlock(&s_sched_lck);
  }
}


static inline void
s_wait_sched(lagopus_chrono_t to) {
  if (likely(s_sched_lck != NULL && s_sched_cnd != NULL)) {
    (void)lagopus_cond_wait(&s_sched_cnd, &s_sched_lck, to);
  }
}


static inline void
s_wakeup_sched(void) {
  if (likely(s_sched_cnd != NULL)) {
    (void)lagopus_cond_notify(&s_sched_cnd, true);
  }
}





static inline lagopus_result_t
s_run_tasks_by_self(const lagopus_callout_task_t * const tasks, size_t n) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(tasks != NULL && n > 0)) {
    lagopus_chrono_t now;
    lagopus_callout_task_t t;
    size_t i;

    WHAT_TIME_IS_IT_NOW_IN_NSEC(now);

    for (i = 0; i < n; i++) {
      /*
       * Change the state of tasks to TASK_STATE_DEQUEUED and set
       * the last execution (start) time as now.
       */
      t = tasks[i];

      s_lock_task(t);
      {
        (void)s_set_task_state_in_table(t, TASK_STATE_DEQUEUED);
        t->m_status = TASK_STATE_DEQUEUED;
        t->m_last_abstime = now;
      }
      s_unlock_task(t);

      (void)s_exec_task(t);
    }

    ret = (lagopus_result_t)n;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_start_callout_main_loop(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;

  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    if (likely(s == GLOBAL_STATE_STARTED)) {
      lagopus_chrono_t timeout = s_idle_interval;
      lagopus_callout_task_t t;

      lagopus_callout_task_t out_tasks[CALLOUT_TASK_MAX * 3];
      size_t n_out_tasks;

      lagopus_callout_task_t urgent_tasks[CALLOUT_TASK_MAX];
      lagopus_result_t sn_urgent_tasks;

      lagopus_callout_task_t idle_tasks[CALLOUT_TASK_MAX];
      lagopus_result_t sn_idle_tasks;

      lagopus_callout_task_t timed_tasks[CALLOUT_TASK_MAX];
      lagopus_result_t sn_timed_tasks;

      lagopus_result_t r;

      lagopus_chrono_t now;
      lagopus_chrono_t now2;
      lagopus_chrono_t next_wakeup;
      lagopus_chrono_t old_next_wakeup;

      int cstate = 0;

      bool do_timed;
      bool do_idle;

      (void)lagopus_mutex_enter_critical(&s_sched_lck, &cstate);
      {
        s_is_stopped = false;
        mbar();

        while (s_do_loop == true) {

          n_out_tasks = 0;

          /*
           * Get the current time.
           */
          WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
          now2 = now - CALLOUT_TASK_SCHED_JITTER;

          /*
           * Then fetch the start time of the timed task in the queue head.
           */
          next_wakeup = s_peek_current_wakeup_time();

          do_idle = (s_idle_proc != NULL &&
                     s_next_idle_abstime < now2) ? true : false;
          do_timed = (next_wakeup > 0LL &&
                      next_wakeup <= now2) ? true : false;

          s_lock_global();
          {

            /*
             * Acquire the global lock to make the task
             * submisson/fetch atomic.
             */

            sn_urgent_tasks = 
                lagopus_bbq_get_n(&s_urgent_tsk_q, (void **)urgent_tasks,
                                  CALLOUT_TASK_MAX, 1LL,
                                  lagopus_callout_task_t,
                                  0LL, NULL);
            sn_idle_tasks = 
                lagopus_bbq_get_n(&s_idle_tsk_q, (void **)idle_tasks,
                                  CALLOUT_TASK_MAX, 1LL,
                                  lagopus_callout_task_t,
                                  0LL, NULL);
          }
          s_unlock_global();

          if (likely((do_idle == true) ||
                     (do_timed == true) ||
                     (sn_urgent_tasks > 0) ||
                     (sn_idle_tasks > 0))) {

            /*
             * We have tasks and/or an idle proc to execute.
             */

            /*
             * Fetch timed tasks to execute.
             */
            sn_timed_tasks = 0;
            if (do_timed == true) {
              sn_timed_tasks = s_get_runnable_timed_task(now, timed_tasks,
                                                         CALLOUT_TASK_MAX,
                                                         &next_wakeup);
            }

            /*
             * Pack the tasks into a buffer.
             */
            if (sn_timed_tasks > 0) {
              (void)memcpy((void *)(out_tasks + n_out_tasks),
                           timed_tasks,
                           (size_t)(sn_timed_tasks) *
                           sizeof(lagopus_callout_task_t));
              n_out_tasks += (size_t)sn_timed_tasks;
            } else if (sn_timed_tasks < 0) {
              /*
               * We can't be treat this as a fatal error. Carry on.
               */
              lagopus_perror(sn_timed_tasks);
              lagopus_msg_error("timed tasks fetch failed.\n");
            }

            if (sn_urgent_tasks > 0) {
              (void)memcpy((void *)(out_tasks + n_out_tasks),
                           urgent_tasks,
                           (size_t)(sn_urgent_tasks) *
                           sizeof(lagopus_callout_task_t));
              n_out_tasks += (size_t)sn_urgent_tasks;
            } else if (sn_urgent_tasks < 0) {
              /*
               * We can't be treat this as a fatal error. Carry on.
               */
              lagopus_perror(sn_urgent_tasks);
              lagopus_msg_error("urgent tasks fetch failed.\n");
            }

            if (sn_idle_tasks > 0) {
              (void)memcpy((void *)(out_tasks + n_out_tasks),
                           idle_tasks,
                           (size_t)(sn_idle_tasks) *
                           sizeof(lagopus_callout_task_t));
              n_out_tasks += (size_t)sn_idle_tasks;
            } else if (sn_idle_tasks < 0) {
              /*
               * We can't be treat this as a fatal error. Carry on.
               */
              lagopus_perror(sn_idle_tasks);
              lagopus_msg_error("idle tasks fetch failed.\n");
            }

            if (n_out_tasks > 0) {
              /*
               * Run/Submit the tasks.
               */
              r = (s_final_task_sched_proc)(out_tasks, n_out_tasks);
              if (unlikely(r <= 0)) {
                /*
                 * We can't be treat this as a fatal error. Carry on.
                 */
                lagopus_perror(r);
                lagopus_msg_error("failed to submit " PFSZ(u) 
                                  " urgent/timed tasks.\n", n_out_tasks);
              }
            }

            if (do_idle == true) {
              if (likely(s_idle_proc(s_idle_proc_arg) ==
                         LAGOPUS_RESULT_OK)) {
                s_next_idle_abstime = now + s_idle_interval;
              } else {
                /*
                 * Stop the main loop and return (clean finish.)
                 */
                s_do_loop = false;
                goto critical_end;
              }
            }

            if (next_wakeup < 0LL) {
              /*
               * Nothing in the timed Q.
               */
              if (s_next_idle_abstime < 0LL) {
                s_next_idle_abstime = now + s_idle_interval;
              }
              next_wakeup = s_next_idle_abstime;
              goto wait_events;
            }

          } else {

            /*
             * Wait any events.
             */

            /*
             * calculate the timeout.
             */
         wait_events:
            timeout = next_wakeup - now;
            if (timeout < 0LL) {
              timeout = 0LL;
            } else {
              timeout = (timeout < s_idle_interval) ?
                  timeout : s_idle_interval;
            }

            /*
             * Atomic update of the wakeup time. Note that usually we
             * don't need following code since the pthread APIs should
             * call the memory barrier internally. But in this
             * particular case we need this since it could take
             * relatively long time to take here and the next pthread
             * related APIs call.
             */
            old_next_wakeup = __sync_fetch_and_add(&s_next_wakeup_abstime, 0);
            if (unlikely(__sync_bool_compare_and_swap(&s_next_wakeup_abstime,
                                                      old_next_wakeup,
                                                      next_wakeup) != true)) {
              /*
               * must not happens since noone modify the old_next_wakeup.
               */
              lagopus_exit_fatal("Someone changed the old wakeup time??\n");
              /* not reached. */
            }

            r = lagopus_bbq_peek(&s_urgent_tsk_q, &t, lagopus_callout_task_t,
                                 timeout);
            if (unlikely(r != LAGOPUS_RESULT_OK &&
                         r != LAGOPUS_RESULT_TIMEDOUT &&
                         r != LAGOPUS_RESULT_WAKEUP_REQUESTED)) {
              lagopus_perror(r);
              lagopus_msg_error("Event wait failure.\n");
              ret = r;
              goto critical_end;
            }

            /*
             * The end of the desired potion of the loop.
             */
          }

        } /* while (s_do_loop == true) */

      }
   critical_end:
      s_is_stopped = true;
      s_wakeup_sched();
      (void)lagopus_mutex_leave_critical(&s_sched_lck, cstate);
  
      if (s_do_loop == false) {
        /*
         * The clean finish.
         */
        ret = LAGOPUS_RESULT_OK;
      }

    } else { /* s == GLOBAL_STATE_STARTED */
      ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
    }
  }

  return ret;
}


static inline lagopus_result_t
s_stop_callout_main_loop(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (s_is_stopped == false) {
    if (s_do_loop == true) {
      /*
       * Stop the main loop first.
       */
      s_do_loop = false;
      mbar();
      (void)lagopus_bbq_wakeup(&s_urgent_tsk_q, -1LL);

      s_lock_sched();
      {
        while (s_is_stopped == false) {
          s_wait_sched(-1LL);
        }
      }
      s_unlock_sched();
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}





/*
 * Exported APIs
 */


lagopus_result_t
lagopus_callout_initialize_handler(size_t n_workers,
                                   lagopus_callout_idle_proc_t proc,
                                   void *arg,
                                   lagopus_chrono_t interval,
                                   lagopus_callout_idle_arg_freeup_proc_t
                                   freeup) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(((proc == NULL) ? true :
              (interval > CALLOUT_IDLE_PROC_MIN_INTERVAL)))) {

    if (likely(n_workers > 0)) {
      if (unlikely((ret = s_create_callout_stage(n_workers)) !=
                   LAGOPUS_RESULT_OK)) {
        goto done;
      }
    }

    s_n_workers = n_workers;
    s_final_task_sched_proc =
        (s_n_workers == 0) ? s_run_tasks_by_self : s_submit_callout_stage;
    s_idle_proc = proc;
    s_idle_proc_arg = arg;
    s_idle_interval = interval;
    s_free_proc = freeup;
    s_do_loop = false;
    s_is_stopped = false;

    s_is_handler_inited = true;

    ret = LAGOPUS_RESULT_OK;

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
lagopus_callout_finalize_handler(void) {
  if (s_is_handler_inited == true) {
    lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

    /*
     * Stop the main loop first then shutdown/wait/destroy the callout
     * stage/workers.
     */
    if (likely((r = s_stop_callout_main_loop()) == LAGOPUS_RESULT_OK)) {
      if (likely(s_n_workers > 0)) {
        r = s_finish_callout_stage(CALLOUT_STAGE_SHUTDOWN_TIMEOUT);
        if (unlikely(r != LAGOPUS_RESULT_OK)) {
          lagopus_perror(r);
          lagopus_msg_warning("failed to stop the callout stage so the "
                              "callout queues still remain.\n");
          goto done;
        }
      }

      s_destroy_all_queued_timed_tasks();
      (void)lagopus_bbq_clear(&s_urgent_tsk_q, true);
      (void)lagopus_bbq_clear(&s_idle_tsk_q, true);

    } else {
      lagopus_perror(r);
      lagopus_msg_warning("failed to stop the callout scheduler main loop.\n");
    }

 done:
    s_is_handler_inited = false;
  }
}


lagopus_result_t
lagopus_callout_start_main_loop(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (s_n_workers > 0) {
      ret = s_start_callout_stage();
      if (likely(ret == LAGOPUS_RESULT_OK)) {
        s_do_loop = true;
        mbar();

        ret = s_start_callout_main_loop();
      }
    } else {
      s_do_loop = true;
      mbar();

      ret = s_start_callout_main_loop();
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


lagopus_result_t
lagopus_callout_stop_main_loop(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    ret = s_stop_callout_main_loop();
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}





/*
 * Task APIs
 */


lagopus_result_t
lagopus_callout_create_task(lagopus_callout_task_t *tptr,
                            size_t sz,
                            const char *name,
                            lagopus_callout_task_proc_t proc,
                            void *arg,
                            lagopus_callout_task_arg_freeup_proc_t freeproc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL)) {
      ret = s_create_task(tptr, sz, name, proc, arg, freeproc);
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


lagopus_result_t
lagopus_callout_submit_task(const lagopus_callout_task_t *tptr,
                            lagopus_chrono_t delay,
                            lagopus_chrono_t interval) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL && *tptr != NULL)) {
      callout_task_state_t st;
      if (likely((st = s_get_task_state_in_table(*tptr)) ==
                 TASK_STATE_CREATED)) {
        ret = s_submit_task(*tptr, delay, interval);
      } else {
        if (st == TASK_STATE_UNKNOWN) {
          ret = LAGOPUS_RESULT_INVALID_OBJECT;
        } else {
          ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


void
lagopus_callout_cancel_task(const lagopus_callout_task_t *tptr) {
  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL && *tptr != NULL)) {
      s_cancel_task(*tptr);
    }
  }
}


lagopus_result_t
lagopus_callout_exec_task_forcibly(const lagopus_callout_task_t *tptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL && *tptr != NULL)) {
      callout_task_state_t st;
      if (likely((st = s_get_task_state_in_table(*tptr)) !=
                 TASK_STATE_UNKNOWN)) {
        ret = s_force_schedule_task(*tptr);
      } else {
        ret = LAGOPUS_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}
