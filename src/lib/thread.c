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

#include "lagopus_apis.h"
#include "lagopus_thread_internal.h"





#define MAX_CPUS	1024	/* Yeah, we mean it. */
#define DEFAULT_THREAD_ALLOC_SZ	(sizeof(lagopus_thread_record))





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static pthread_attr_t s_attr;
static lagopus_hashmap_t s_thd_tbl;
static lagopus_hashmap_t s_alloc_tbl;
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
static size_t s_cpu_set_sz;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
static void s_ctors(void) __attr_constructor__(106);
static void s_dtors(void) __attr_destructor__(106);





static void
s_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&s_thd_tbl);
  lagopus_hashmap_atfork_child(&s_alloc_tbl);
}


static void
s_once_proc(void) {
  int st;
  lagopus_result_t r;

  if ((st = pthread_attr_init(&s_attr)) == 0) {
    if ((st = pthread_attr_setdetachstate(&s_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0) {
      errno = st;
      perror("pthread_attr_setdetachstate");
      lagopus_exit_fatal("can't initialize detached thread attr.\n");
    }
  } else {
    errno = st;
    perror("pthread_attr_init");
    lagopus_exit_fatal("can't initialize thread attr.\n");
  }

#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  s_cpu_set_sz = CPU_ALLOC_SIZE(MAX_CPUS);
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */

  if ((r = lagopus_hashmap_create(&s_thd_tbl, LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                  NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the thread table.\n");
  }
  if ((r = lagopus_hashmap_create(&s_alloc_tbl, LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                  NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the thread allocation table.\n");
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);
}


static inline void
s_init(void) {
  lagopus_heapcheck_module_initialize();
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(10, "The thread module is initialized.\n");
}


static void
s_final(void) {
  lagopus_hashmap_destroy(&s_thd_tbl, true);
  lagopus_hashmap_destroy(&s_alloc_tbl, true);
}


static void
s_dtors(void) {
  if (lagopus_module_is_unloading() &&
      lagopus_module_is_finalized_cleanly()) {
    s_final();

    lagopus_msg_debug(10, "The thread module is finalized.\n");
  } else {
    lagopus_msg_debug(10, "The thread module is not finalized "
                      "because of module finalization problem.\n");
  }
}





static inline void
s_add_thd(const lagopus_thread_t thd) {
  void *val = (void *)true;
  (void)lagopus_hashmap_add(&s_thd_tbl, (void *)thd, &val, true);
}


static inline void
s_alloc_mark_thd(const lagopus_thread_t thd) {
  void *val = (void *)true;
  (void)lagopus_hashmap_add(&s_alloc_tbl, (void *)thd, &val, true);
}


static inline void
s_delete_thd(const lagopus_thread_t thd) {
  (void)lagopus_hashmap_delete(&s_thd_tbl, (void *)thd, NULL, true);
  (void)lagopus_hashmap_delete(&s_alloc_tbl, (void *)thd, NULL, true);
}


static inline bool
s_is_thd(const lagopus_thread_t thd) {
  void *val;
  lagopus_result_t r = lagopus_hashmap_find(&s_thd_tbl, (void *)thd, &val);
  return (r == LAGOPUS_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline bool
s_is_alloc_marked(const lagopus_thread_t thd) {
  void *val;
  lagopus_result_t r = lagopus_hashmap_find(&s_alloc_tbl, (void *)thd, &val);
  return (r == LAGOPUS_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline void
s_op_lock(const lagopus_thread_t thd) {
  if (thd != NULL) {
    (void)lagopus_mutex_lock(&(thd->m_op_lock));
  }
}


static inline void
s_op_unlock(const lagopus_thread_t thd) {
  if (thd != NULL) {
    (void)lagopus_mutex_unlock(&(thd->m_op_lock));
  }
}


static inline void
s_wait_lock(const lagopus_thread_t thd) {
  if (thd != NULL) {
    (void)lagopus_mutex_lock(&(thd->m_wait_lock));
  }
}


static inline void
s_wait_unlock(const lagopus_thread_t thd) {
  if (thd != NULL) {
    (void)lagopus_mutex_unlock(&(thd->m_wait_lock));
  }
}


static inline void
s_cancel_lock(const lagopus_thread_t thd) {
  if (thd != NULL) {
    (void)lagopus_mutex_lock(&(thd->m_cancel_lock));
  }
}


static inline void
s_cancel_unlock(const lagopus_thread_t thd) {
  if (thd != NULL) {
    (void)lagopus_mutex_unlock(&(thd->m_cancel_lock));
  }
}





static inline lagopus_result_t
s_initialize(lagopus_thread_t thd,
             lagopus_thread_main_proc_t mainproc,
             lagopus_thread_finalize_proc_t finalproc,
             lagopus_thread_freeup_proc_t freeproc,
             const char *name,
             void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thd != NULL) {
    (void)memset((void *)thd, 0, sizeof(*thd));
    s_add_thd(thd);
    if (((ret = lagopus_mutex_create(&(thd->m_wait_lock))) ==
         LAGOPUS_RESULT_OK) &&
        ((ret = lagopus_mutex_create(&(thd->m_op_lock))) ==
         LAGOPUS_RESULT_OK) &&
        ((ret = lagopus_mutex_create(&(thd->m_cancel_lock))) ==
         LAGOPUS_RESULT_OK) &&
        ((ret = lagopus_mutex_create(&(thd->m_finalize_lock))) ==
         LAGOPUS_RESULT_OK) &&
        ((ret = lagopus_cond_create(&(thd->m_wait_cond))) ==
         LAGOPUS_RESULT_OK) &&
        ((ret = lagopus_cond_create(&(thd->m_startup_cond))) ==
         LAGOPUS_RESULT_OK) &&
        ((ret = lagopus_cond_create(&(thd->m_finalize_cond))) ==
         LAGOPUS_RESULT_OK)) {
      thd->m_arg = arg;
      if (IS_VALID_STRING(name) == true) {
        snprintf(thd->m_name, sizeof(thd->m_name), "%s", name);
      }
      thd->m_creator_pid = getpid();
      thd->m_pthd = LAGOPUS_INVALID_THREAD;

      thd->m_main_proc = mainproc;
      thd->m_final_proc = finalproc;
      thd->m_freeup_proc = freeproc;
      thd->m_result_code = LAGOPUS_RESULT_NOT_STARTED;

      thd->m_is_started = false;
      thd->m_is_activated = false;
      thd->m_is_canceled = false;
      thd->m_is_finalized = false;
      thd->m_is_destroying = false;

      thd->m_do_autodelete = false;
      thd->m_startup_sync_done = false;
      thd->m_n_finalized_count = 0LL;

      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_finalize(lagopus_thread_t thd, bool is_canceled,
           lagopus_result_t rcode) {
  if (likely(thd != NULL)) {
    lagopus_thread_t cthd = thd;
    bool is_valid = false;
    lagopus_result_t r = lagopus_thread_is_valid(&cthd, &is_valid);

    if (likely(r == LAGOPUS_RESULT_OK && is_valid == true)) {
      bool do_autodelete = false;
      int o_cancel_state;

      (void)lagopus_mutex_enter_critical(&(thd->m_finalize_lock),
                                         &o_cancel_state);
      {

        lagopus_msg_debug(5, "enter: %s\n",
                          (is_canceled == true) ? "canceled" : "exit");

        s_cancel_lock(thd);
        {
          thd->m_is_canceled = is_canceled;
        }
        s_cancel_unlock(thd);

        s_op_lock(thd);
        {
          thd->m_result_code = rcode;
        }
        s_op_unlock(thd);

        s_wait_lock(thd);
        {
          if (thd->m_final_proc != NULL) {
            if (likely(thd->m_n_finalized_count == 0)) {
              thd->m_final_proc(&thd, is_canceled, thd->m_arg);
            } else {
              lagopus_msg_warning("the thread is already %s, count "
                                  PFSZ(u) " (now %s)\n",
                                  (thd->m_is_canceled == false) ?
                                  "exit" : "canceled",
                                  thd->m_n_finalized_count,
                                  (is_canceled == false) ?
                                  "exit" : "canceled");
            }
          }
          thd->m_pthd = LAGOPUS_INVALID_THREAD;
          do_autodelete = thd->m_do_autodelete;
          thd->m_is_finalized = true;
          thd->m_is_activated = false;
          thd->m_n_finalized_count++;
          (void)lagopus_cond_notify(&(thd->m_wait_cond), true);
        }
        s_wait_unlock(thd);

        (void)lagopus_cond_notify(&(thd->m_finalize_cond), true);

      }
      (void)lagopus_mutex_leave_critical(&(thd->m_finalize_lock),
                                         o_cancel_state);

      if (do_autodelete == true) {
        lagopus_thread_destroy(&thd);
      }

      /*
       * NOTE:
       *
       *	Don't call pthread_exit() at here because now it
       *	triggers the cancel handler as of current cancellation
       *	handling.
       */
    }
  }
}


static inline void
s_delete(lagopus_thread_t thd) {
  if (thd != NULL) {
    if (s_is_thd(thd) == true) {
      if (s_is_alloc_marked(thd) == true) {
        lagopus_msg_debug(5, "free %p\n", (void *)thd);
        free((void *)thd);
      } else {
        if (lagopus_heapcheck_is_in_heap((const void *)thd) == true) {
          lagopus_msg_debug(5, "%p is a thread object NOT allocated by the "
                            "lagopus_thread_create(). If you want to free(3) "
                            "this by calling the lagopus_thread_destroy(), "
                            "call lagopus_thread_free_when_destroy(%p).\n",
                            (void *)thd, (void *)thd);
        } else {
          lagopus_msg_debug(10, "A thread was created in an address %p and it "
                            "seems not in the heap area.\n", (void *)thd);
        }
      }

      s_delete_thd(thd);
    }
  }
}


static inline void
s_destroy(lagopus_thread_t *thdptr, bool is_clean_finish) {
  if (thdptr != NULL && *thdptr != NULL) {

    s_wait_lock(*thdptr);
    {

      if (is_clean_finish == true) {

        if ((*thdptr)->m_is_destroying == false) {
          (*thdptr)->m_is_destroying = true;

          if ((*thdptr)->m_freeup_proc != NULL) {
            (*thdptr)->m_freeup_proc(thdptr, (*thdptr)->m_arg);
          }
        }

      }

      if ((*thdptr)->m_cpusetptr != NULL) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
        CPU_FREE((*thdptr)->m_cpusetptr);
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
        (*thdptr)->m_cpusetptr = NULL;
      }

      if ((*thdptr)->m_op_lock != NULL) {
        (void)lagopus_mutex_destroy(&((*thdptr)->m_op_lock));
        (*thdptr)->m_op_lock = NULL;
      }
      if ((*thdptr)->m_cancel_lock != NULL) {
        (void)lagopus_mutex_destroy(&((*thdptr)->m_cancel_lock));
        (*thdptr)->m_cancel_lock = NULL;
      }
      if ((*thdptr)->m_finalize_lock != NULL) {
        (void)lagopus_mutex_destroy(&((*thdptr)->m_finalize_lock));
        (*thdptr)->m_finalize_lock = NULL;
      }
      if ((*thdptr)->m_wait_cond != NULL) {
        (void)lagopus_cond_destroy(&((*thdptr)->m_wait_cond));
        (*thdptr)->m_wait_cond = NULL;
      }
      if ((*thdptr)->m_startup_cond != NULL) {
        (void)lagopus_cond_destroy(&((*thdptr)->m_startup_cond));
        (*thdptr)->m_startup_cond = NULL;
      }
      if ((*thdptr)->m_finalize_cond != NULL) {
        (void)lagopus_cond_destroy(&((*thdptr)->m_finalize_cond));
        (*thdptr)->m_finalize_cond = NULL;
      }

    }
    s_wait_unlock(*thdptr);

    if ((*thdptr)->m_wait_lock != NULL) {
      (void)lagopus_mutex_destroy(&((*thdptr)->m_wait_lock));
    }

    s_delete(*thdptr);
  }
}





static void
s_pthd_cancel_handler(void *ptr) {
  if (ptr != NULL) {
    lagopus_thread_t thd = (lagopus_thread_t)ptr;
    s_finalize(thd, true, LAGOPUS_RESULT_INTERRUPTED);
  }
}


static void *
s_pthd_entry_point(void *ptr) {
  if (likely(ptr != NULL)) {

    pthread_cleanup_push(s_pthd_cancel_handler, ptr);
    {
      int o_cancel_state;
      lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
      lagopus_thread_t thd = (lagopus_thread_t)ptr;
      const lagopus_thread_t *tptr =
        (const lagopus_thread_t *)&thd;

      (void)lagopus_mutex_enter_critical(&(thd->m_wait_lock), &o_cancel_state);
      {

        s_cancel_lock(thd);
        {
          thd->m_is_canceled = false;
        }
        s_cancel_unlock(thd);

        thd->m_is_finalized = false;
        thd->m_is_activated = true;
        thd->m_is_started = true;
        (void)lagopus_cond_notify(&(thd->m_startup_cond), true);

        /*
         * The notifiction is done and then the parent thread send us
         * "an ACK". Wait for it.
         */
     sync_done_check:
        mbar();
        if (thd->m_startup_sync_done == false) {
          ret = lagopus_cond_wait(&(thd->m_startup_cond),
                                  &(thd->m_wait_lock),
                                  -1);

          if (ret == LAGOPUS_RESULT_OK) {
            goto sync_done_check;
          } else {
            lagopus_perror(ret);
            lagopus_msg_error("startup synchronization failed with the"
                              "parent thread.\n");
          }
        }

      }
      (void)lagopus_mutex_leave_critical(&(thd->m_wait_lock), o_cancel_state);

      ret = thd->m_main_proc(tptr, thd->m_arg);

      /*
       * NOTE:
       *
       *	This very moment this thread could be cancelled and no
       *	correct result is set. Even it is impossible to
       *	prevent it completely, try to prevent it anyway.
       */
      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);

      /*
       * We got through anyhow.
       */
      s_finalize(thd, false, ret);

      /*
       * NOTE
       *
       *	Don't call pthread_exit() at here or the cancellation
       *	handler will be called.
       */
    }
    pthread_cleanup_pop(0);

  }

  return NULL;
}





lagopus_result_t
lagopus_thread_create_with_size(lagopus_thread_t *thdptr,
                                size_t alloc_size,
                                lagopus_thread_main_proc_t mainproc,
                                lagopus_thread_finalize_proc_t finalproc,
                                lagopus_thread_freeup_proc_t freeproc,
                                const char *name,
                                void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      mainproc != NULL) {
    lagopus_thread_t thd;

    if (*thdptr == NULL) {
      size_t sz = (DEFAULT_THREAD_ALLOC_SZ > alloc_size) ?
          DEFAULT_THREAD_ALLOC_SZ : alloc_size;
      thd = (lagopus_thread_t)malloc(sz);
      if (thd != NULL) {
        *thdptr = thd;
        s_alloc_mark_thd(thd);
      } else {
        *thdptr = NULL;
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto done;
      }
    } else {
      thd = *thdptr;
    }

    ret = s_initialize(thd, mainproc, finalproc, freeproc, name, arg);
    if (ret != LAGOPUS_RESULT_OK) {
      s_destroy(&thd, false);
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


lagopus_result_t
lagopus_thread_create(lagopus_thread_t *thdptr,
                      lagopus_thread_main_proc_t mainproc,
                      lagopus_thread_finalize_proc_t finalproc,
                      lagopus_thread_freeup_proc_t freeproc,
                      const char *name,
                      void *arg) {
  return lagopus_thread_create_with_size(thdptr, 0,
                                         mainproc, finalproc, freeproc,
                                         name, arg);
}


lagopus_result_t
lagopus_thread_start(const lagopus_thread_t *thdptr,
                     bool autodelete) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      int st;

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_is_activated == false) {
          (*thdptr)->m_do_autodelete = autodelete;
          errno = 0;
          (*thdptr)->m_pthd = LAGOPUS_INVALID_THREAD;
          if ((st = pthread_create((pthread_t *)&((*thdptr)->m_pthd),
                                   &s_attr,
                                   s_pthd_entry_point,
                                   (void *)*thdptr)) == 0) {
#ifdef HAVE_PTHREAD_SETNAME_NP
            if (IS_VALID_STRING((*thdptr)->m_name) == true) {
              (void)pthread_setname_np((*thdptr)->m_pthd, (*thdptr)->m_name);
            }
#endif /* HAVE_PTHREAD_SETNAME_NP */
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
            if ((*thdptr)->m_cpusetptr != NULL &&
                (st =
                   pthread_setaffinity_np((*thdptr)->m_pthd,
                                          s_cpu_set_sz,
                                          (*thdptr)->m_cpusetptr)) != 0) {
              int s_errno = errno;
              errno = st;
              lagopus_perror(LAGOPUS_RESULT_POSIX_API_ERROR);
              lagopus_msg_warning("Can't set cpu affinity.\n");
              errno = s_errno;
            }
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
            (*thdptr)->m_is_activated = false;
            (*thdptr)->m_is_finalized = false;
            (*thdptr)->m_is_destroying = false;
            (*thdptr)->m_is_canceled = false;
            (*thdptr)->m_is_started = false;
            (*thdptr)->m_startup_sync_done = false;
            (*thdptr)->m_n_finalized_count = 0LL;
            mbar();

            /*
             * Wait the spawned thread starts.
             */

          startcheck:
            mbar();
            if ((*thdptr)->m_is_started == false) {

              /*
               * Note that very here, very this moment the spawned
               * thread starts to run since this thread sleeps via
               * calling of the pthread_cond_wait() that implies
               * release of the lock.
               */

              ret = lagopus_cond_wait(&((*thdptr)->m_startup_cond),
                                      &((*thdptr)->m_wait_lock),
                                      -1);

              if (ret == LAGOPUS_RESULT_OK) {
                goto startcheck;
              } else {
                goto unlock;
              }
            }

            /*
             * The newly created thread notified its start. Then "send
             * an ACK" to the thread to notify that the thread can do
             * anything, including its deletion.
             */
            (*thdptr)->m_startup_sync_done = true;
            (void)lagopus_cond_notify(&((*thdptr)->m_startup_cond), true);

            ret = LAGOPUS_RESULT_OK;

          } else {
            errno = st;
            ret = LAGOPUS_RESULT_POSIX_API_ERROR;
          }
        } else {
          ret = LAGOPUS_RESULT_ALREADY_EXISTS;
        }
      }
    unlock:
      s_wait_unlock(*thdptr);

    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_cancel(const lagopus_thread_t *thdptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {

        s_wait_lock(*thdptr);
        {
          if ((*thdptr)->m_is_activated == true) {

            s_cancel_lock(*thdptr);
            {
              if ((*thdptr)->m_is_canceled == false &&
                  (*thdptr)->m_pthd != LAGOPUS_INVALID_THREAD) {
                int st;

                errno = 0;
                if ((st = pthread_cancel((*thdptr)->m_pthd)) == 0) {
                  ret = LAGOPUS_RESULT_OK;
                } else {
                  errno = st;
                  ret = LAGOPUS_RESULT_POSIX_API_ERROR;
                }
              } else {
                ret = LAGOPUS_RESULT_OK;
              }
            }
            s_cancel_unlock(*thdptr);

          } else {
            ret = LAGOPUS_RESULT_OK;
          }
        }
        s_wait_unlock(*thdptr);

        /*
         * Very here, very this moment the s_finalize() would start to
         * run.
         */

      } else {
        ret = LAGOPUS_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = LAGOPUS_RESULT_NOT_OWNER;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_wait(const lagopus_thread_t *thdptr,
                    lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {
        if ((*thdptr)->m_do_autodelete == false) {

          int o_cancel_state;

          (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                       &o_cancel_state);
          {

            s_wait_lock(*thdptr);
            {
           waitcheck:
              mbar();
              if ((*thdptr)->m_is_activated == true) {
                ret = lagopus_cond_wait(&((*thdptr)->m_wait_cond),
                                        &((*thdptr)->m_wait_lock),
                                        nsec);
                if (ret == LAGOPUS_RESULT_OK) {
                  goto waitcheck;
                }
              } else {
                ret = LAGOPUS_RESULT_OK;
              }
            }
            s_wait_unlock(*thdptr);

            if ((*thdptr)->m_is_started == true) {

              (void)lagopus_mutex_lock(&((*thdptr)->m_finalize_lock));
              {
                mbar();
             finalcheck:
                if ((*thdptr)->m_n_finalized_count == 0) {
                  ret = lagopus_cond_wait(&((*thdptr)->m_finalize_cond),
                                          &((*thdptr)->m_finalize_lock),
                                          nsec);
                  if (ret == LAGOPUS_RESULT_OK) {
                    goto finalcheck;
                  }
                }
              }
              (void)lagopus_mutex_unlock(&((*thdptr)->m_finalize_lock));

            }

          }
          (void)pthread_setcancelstate(o_cancel_state, NULL);

        } else {
          ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
        }
      } else {
        ret = LAGOPUS_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = LAGOPUS_RESULT_NOT_OWNER;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_thread_destroy(lagopus_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL &&
      s_is_thd(*thdptr) == true) {
    (void)lagopus_thread_cancel(thdptr);
    (void)lagopus_thread_wait(thdptr, -1);

    s_destroy(thdptr, true);
    *thdptr = NULL;
  }
}


static inline lagopus_result_t
s_get_pthdid(const lagopus_thread_t *thdptr,
             pthread_t *tidptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((*thdptr)->m_is_started == true) {
    *tidptr = (*thdptr)->m_pthd;
    if ((*thdptr)->m_pthd != LAGOPUS_INVALID_THREAD) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_ALREADY_HALTED;
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_STARTED;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_get_pthread_id(const lagopus_thread_t *thdptr,
                              pthread_t *tidptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      tidptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      int o_cancel_state;
      *tidptr = LAGOPUS_INVALID_THREAD;

      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);
      {

        s_wait_lock(*thdptr);
        {

          ret = s_get_pthdid(thdptr, tidptr);

        }
        s_wait_unlock(*thdptr);

      }
      (void)pthread_setcancelstate(o_cancel_state, NULL);

    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_set_cpu_affinity(const lagopus_thread_t *thdptr,
                                int cpu) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_cpusetptr == NULL) {
          (*thdptr)->m_cpusetptr = CPU_ALLOC(MAX_CPUS);
          if ((*thdptr)->m_cpusetptr == NULL) {
            ret = LAGOPUS_RESULT_NO_MEMORY;
            goto done;
          }
          CPU_ZERO_S(s_cpu_set_sz, (*thdptr)->m_cpusetptr);
        }

        if (cpu >= 0) {
          CPU_SET_S((size_t)cpu, s_cpu_set_sz, (*thdptr)->m_cpusetptr);
        } else {
          CPU_ZERO_S(s_cpu_set_sz, (*thdptr)->m_cpusetptr);
        }

        ret = s_get_pthdid(thdptr, &tid);
        if (ret == LAGOPUS_RESULT_OK) {
          int st;
          if ((st = pthread_setaffinity_np(tid,
                                           s_cpu_set_sz,
                                           (*thdptr)->m_cpusetptr)) == 0) {
            ret = LAGOPUS_RESULT_OK;
          } else {
            errno = st;
            ret = LAGOPUS_RESULT_POSIX_API_ERROR;
          }
        } else {
          if (ret == LAGOPUS_RESULT_NOT_STARTED) {
            ret = LAGOPUS_RESULT_OK;
          }
        }

      }
    done:
      s_wait_unlock(*thdptr);

    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  return LAGOPUS_RESULT_OK;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


lagopus_result_t
lagopus_thread_get_cpu_affinity(const lagopus_thread_t *thdptr) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {

        if ((ret = s_get_pthdid(thdptr, &tid)) == LAGOPUS_RESULT_OK) {
          int st;
          cpu_set_t *cur_set = CPU_ALLOC(MAX_CPUS);

          if (cur_set == NULL) {
            ret = LAGOPUS_RESULT_NO_MEMORY;
            goto done;
          }
          CPU_ZERO_S(s_cpu_set_sz, (*thdptr)->m_cpusetptr);

          if ((st = pthread_getaffinity_np(tid, s_cpu_set_sz, cur_set)) == 0) {
            int i;
            int cpu = -INT_MAX;

            for (i = 0; i < MAX_CPUS; i++) {
              if (CPU_ISSET_S((unsigned)i, s_cpu_set_sz, cur_set)) {
                cpu = (lagopus_result_t)i;
                break;
              }
            }

            if (cpu != -INT_MAX) {

#if SIZEOF_PTHREAD_T == SIZEOF_INT64_T
#define TIDFMT "0x" PFTIDS(016, x)
#elif SIZEOF_PTHREAD_T == SIZEOF_INT
#define TIDFMT "0x" PFTIDS(08, x)
#endif /* SIZEOF_PTHREAD_T == SIZEOF_INT64_T ... */

              if ((*thdptr)->m_cpusetptr != NULL &&
                  (!(CPU_ISSET_S((unsigned)cpu, s_cpu_set_sz,
                                 (*thdptr)->m_cpusetptr)))) {
                const char *name =
                    (IS_VALID_STRING((*thdptr)->m_name) == true) ?
                    (*thdptr)->m_name : "???";
              

                lagopus_msg_warning("Thread " TIDFMT " \"%s\" is running on "
                                    "CPU %d, but is not specified to run "
                                    "on it.\n",
                                    tid, name, cpu);
              }
                  
              ret = (lagopus_result_t)cpu;

            } else {
              lagopus_msg_error("Thread " TIDFMT " is running on unknown "
                                "CPU??\n", tid);
              ret = LAGOPUS_RESULT_POSIX_API_ERROR;
            }

#undef TIDFMT

          } else {
            errno = st;
            ret = LAGOPUS_RESULT_POSIX_API_ERROR;
          }

          CPU_FREE(cur_set);

        } else {	/* (ret = s_get_pthdid(thdptr, &tid)) ... */

          if (ret == LAGOPUS_RESULT_NOT_STARTED) {
            if ((*thdptr)->m_cpusetptr != NULL) {
              int i;
              int cpu = -INT_MAX;

              for (i = 0; i < MAX_CPUS; i++) {
                if (CPU_ISSET_S((unsigned)i, s_cpu_set_sz,
                                (*thdptr)->m_cpusetptr)) {
                  cpu = (lagopus_result_t)i;
                  break;
                }
              }

              if (cpu != -INT_MAX) {
                ret = (lagopus_result_t)cpu;
              } else {
                ret = LAGOPUS_RESULT_NOT_DEFINED;
              }
            } else {
              ret = LAGOPUS_RESULT_NOT_DEFINED;
            }
          }

        }

      }
    done:
      s_wait_unlock(*thdptr);

    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  return 0;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


lagopus_result_t
lagopus_thread_set_result_code(const lagopus_thread_t *thdptr,
                               lagopus_result_t code) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      s_op_lock(*thdptr);
      {
        (*thdptr)->m_result_code = code;
        ret = LAGOPUS_RESULT_OK;
      }
      s_op_unlock(*thdptr);

    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_get_result_code(const lagopus_thread_t *thdptr,
                               lagopus_result_t *codeptr,
                               lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      codeptr != NULL) {

    *codeptr = LAGOPUS_RESULT_ANY_FAILURES;

    if (s_is_thd(*thdptr) == true) {
      if ((ret = lagopus_thread_wait(thdptr, nsec)) == LAGOPUS_RESULT_OK) {

        s_op_lock(*thdptr);
        {
          *codeptr = (*thdptr)->m_result_code;
          ret = LAGOPUS_RESULT_OK;
        }
        s_op_unlock(*thdptr);

      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_is_canceled(const lagopus_thread_t *thdptr,
                           bool *retptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    *retptr = false;

    if (s_is_thd(*thdptr) == true) {

      s_cancel_lock(*thdptr);
      {
        *retptr = (*thdptr)->m_is_canceled;
        ret = LAGOPUS_RESULT_OK;
      }
      s_cancel_unlock(*thdptr);

    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_free_when_destroy(lagopus_thread_t *thdptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      s_alloc_mark_thd(*thdptr);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_thread_is_valid(const lagopus_thread_t *thdptr,
                        bool *retptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      *retptr = true;
      ret = LAGOPUS_RESULT_OK;
    } else {
      *retptr = false;
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_thread_atfork_child(const lagopus_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      (void)lagopus_mutex_reinitialize(&((*thdptr)->m_op_lock));
      (void)lagopus_mutex_reinitialize(&((*thdptr)->m_wait_lock));
      (void)lagopus_mutex_reinitialize(&((*thdptr)->m_cancel_lock));
      (void)lagopus_mutex_reinitialize(&((*thdptr)->m_finalize_lock));
    }
  }
}


void
lagopus_thread_module_initialize(void) {
  s_init();
}


void
lagopus_thread_module_finalize(void) {
  s_final();
}
