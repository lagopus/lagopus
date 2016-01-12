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
#include "lagopus_config.h"





struct lagopus_mutex_record {
  pthread_mutex_t m_mtx;
  pid_t m_creator_pid;
  lagopus_mutex_type_t m_type;
};


struct lagopus_rwlock_record {
  pthread_rwlock_t m_rwl;
  pid_t m_creator_pid;
};


struct lagopus_cond_record {
  pthread_cond_t m_cond;
  pid_t m_creator_pid;
};


struct lagopus_barrier_record {
  pthread_barrier_t m_barrier;
  pid_t m_creator_pid;
};


typedef int (*notify_proc_t)(pthread_cond_t *cnd);





static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static notify_proc_t s_notify_single_proc = pthread_cond_signal;
static notify_proc_t s_notify_all_proc = pthread_cond_broadcast;

static pthread_mutexattr_t s_recur_attr;





static void s_ctors(void) __attr_constructor__(102);
static void s_dtors(void) __attr_destructor__(102);





static void
s_once_proc(void) {

#ifdef LAGOPUS_OS_LINUX
#define RECURSIVE_MUTEX_ATTR PTHREAD_MUTEX_RECURSIVE_NP
#else
#define RECURSIVE_MUTEX_ATTR PTHREAD_MUTEX_RECURSIVE
#endif /* LAGOPUS_OS_LINUX */

  (void)pthread_mutexattr_init(&s_recur_attr);
  if (pthread_mutexattr_settype(&s_recur_attr, RECURSIVE_MUTEX_ATTR) != 0) {
    lagopus_exit_fatal("can't initialize a recursive mutex attribute.\n");
  }
#undef RECURSIVE_MUTEX_ATTR
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(10, "The mutex/lock APIs are initialized.\n");
}


static inline void
s_final(void) {
  (void)pthread_mutexattr_destroy(&s_recur_attr);
}


static void
s_dtors(void) {
  if (lagopus_module_is_unloading() &&
      lagopus_module_is_finalized_cleanly()) {
    s_final();

    lagopus_msg_debug(10, "The mutex/lock APIs are finalized.\n");
  } else {
    lagopus_msg_debug(10, "The mutex/lock APIs is not finalized "
                      "because of module finalization problem.\n");
  }
}





static inline lagopus_result_t
s_create(lagopus_mutex_t *mtxptr, lagopus_mutex_type_t type) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_mutex_t mtx = NULL;

  if (mtxptr != NULL) {
    *mtxptr = NULL;
    mtx = (lagopus_mutex_t)malloc(sizeof(*mtx));
    if (mtx != NULL) {
      int st;
      pthread_mutexattr_t *attr = NULL;

      switch (type) {
        case LAGOPUS_MUTEX_TYPE_DEFAULT: {
          break;
        }
        case LAGOPUS_MUTEX_TYPE_RECURSIVE: {
          attr = &s_recur_attr;
          break;
        }
        default: {
          lagopus_exit_fatal("Invalid lagopus mutex type (%d).\n",
                             (int)type);
          /* not reached. */
          break;
        }
      }

      errno = 0;
      if ((st = pthread_mutex_init(&(mtx->m_mtx), attr)) == 0) {
        mtx->m_type = type;
        mtx->m_creator_pid = getpid();
        *mtxptr = mtx;
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
    if (ret != LAGOPUS_RESULT_OK) {
      free((void *)mtx);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





lagopus_result_t
lagopus_mutex_create(lagopus_mutex_t *mtxptr) {
  return s_create(mtxptr, LAGOPUS_MUTEX_TYPE_DEFAULT);
}


lagopus_result_t
lagopus_mutex_create_recursive(lagopus_mutex_t *mtxptr) {
  return s_create(mtxptr, LAGOPUS_MUTEX_TYPE_RECURSIVE);
}


void
lagopus_mutex_destroy(lagopus_mutex_t *mtxptr) {
  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    if ((*mtxptr)->m_creator_pid == getpid()) {
      (void)pthread_mutex_destroy(&((*mtxptr)->m_mtx));
    }
    free((void *)*mtxptr);
    *mtxptr = NULL;
  }
}


lagopus_result_t
lagopus_mutex_reinitialize(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    pthread_mutexattr_t *attr = NULL;

    switch ((*mtxptr)->m_type) {
      case LAGOPUS_MUTEX_TYPE_DEFAULT: {
        break;
      }
      case LAGOPUS_MUTEX_TYPE_RECURSIVE: {
        attr = &s_recur_attr;
        break;
      }
      default: {
        lagopus_exit_fatal("Invalid lagopus mutex type (%d).\n",
                           (*mtxptr)->m_type);
        /* not reached. */
        break;
      }
    }

    errno = 0;
    if ((st = pthread_mutex_init(&((*mtxptr)->m_mtx), attr)) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_mutex_get_type(lagopus_mutex_t *mtxptr, lagopus_mutex_type_t *tptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL && *mtxptr != NULL && tptr != NULL) {
    *tptr = (*mtxptr)->m_type;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_mutex_lock(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_mutex_trylock(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    if ((st = pthread_mutex_trylock(&((*mtxptr)->m_mtx))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = LAGOPUS_RESULT_BUSY;
      } else {
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

#ifdef HAVE_PTHREAD_MUTEX_TIMEDLOCK
lagopus_result_t
lagopus_mutex_timedlock(lagopus_mutex_t *mtxptr,
                        lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      lagopus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_mutex_timedlock(&((*mtxptr)->m_mtx),
                                        &ts)) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = LAGOPUS_RESULT_TIMEDOUT;
        } else {
          ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
#endif /* HAVE_PTHREAD_MUTEX_TIMEDLOCK */


lagopus_result_t
lagopus_mutex_unlock(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    /*
     * The caller must have this mutex locked.
     */
    if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_mutex_enter_critical(lagopus_mutex_t *mtxptr, int *ostateptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL && *mtxptr != NULL &&
      ostateptr != NULL) {
    int st;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     ostateptr)) == 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_mutex_leave_critical(lagopus_mutex_t *mtxptr, int ostate) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL && *mtxptr != NULL &&
      (ostate == PTHREAD_CANCEL_ENABLE ||
       ostate == PTHREAD_CANCEL_DISABLE)) {
    int st;

    /*
     * The caller must have this mutex locked.
     */
    if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
      if ((st = pthread_setcancelstate(ostate, NULL)) == 0) {
          ret = LAGOPUS_RESULT_OK;
          if (ostate == PTHREAD_CANCEL_ENABLE) {
            pthread_testcancel();
          }
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





lagopus_result_t
lagopus_rwlock_create(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_rwlock_t rwl = NULL;

  if (rwlptr != NULL) {
    *rwlptr = NULL;
    rwl = (lagopus_rwlock_t)malloc(sizeof(*rwl));
    if (rwl != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_rwlock_init(&(rwl->m_rwl), NULL)) == 0) {
        rwl->m_creator_pid = getpid();
        *rwlptr = rwl;
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
    if (ret != LAGOPUS_RESULT_OK) {
      free((void *)rwl);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_rwlock_destroy(lagopus_rwlock_t *rwlptr) {
  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    if ((*rwlptr)->m_creator_pid == getpid()) {
      (void)pthread_rwlock_destroy(&((*rwlptr)->m_rwl));
    }
    free((void *)*rwlptr);
    *rwlptr = NULL;
  }
}


lagopus_result_t
lagopus_rwlock_reinitialize(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    errno = 0;
    if ((st = pthread_rwlock_init(&((*rwlptr)->m_rwl), NULL)) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_reader_lock(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_reader_trylock(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_tryrdlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = LAGOPUS_RESULT_BUSY;
      } else {
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_reader_timedlock(lagopus_rwlock_t *rwlptr,
                                lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      lagopus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_rwlock_timedrdlock(&((*rwlptr)->m_rwl),
                                           &ts)) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = LAGOPUS_RESULT_TIMEDOUT;
        } else {
          ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_writer_lock(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_writer_trylock(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_trywrlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = LAGOPUS_RESULT_BUSY;
      } else {
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_writer_timedlock(lagopus_rwlock_t *rwlptr,
                                lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      lagopus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_rwlock_timedwrlock(&((*rwlptr)->m_rwl),
                                           &ts)) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = LAGOPUS_RESULT_TIMEDOUT;
        } else {
          ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_unlock(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    /*
     * The caller must have this rwlock locked.
     */
    if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_reader_enter_critical(lagopus_rwlock_t *rwlptr,
                                     int *ostateptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL && *rwlptr != NULL &&
      ostateptr != NULL) {
    int st;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     ostateptr)) == 0) {
      if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_writer_enter_critical(lagopus_rwlock_t *rwlptr,
                                     int *ostateptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL && *rwlptr != NULL &&
      ostateptr != NULL) {
    int st;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     ostateptr)) == 0) {
      if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_leave_critical(lagopus_rwlock_t *rwlptr, int ostate) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL && *rwlptr != NULL &&
      (ostate == PTHREAD_CANCEL_ENABLE ||
       ostate == PTHREAD_CANCEL_DISABLE)) {
    int st;

    /*
     * The caller must have this rwlock locked.
     */
    if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
      if ((st = pthread_setcancelstate(ostate, NULL)) == 0) {
        ret = LAGOPUS_RESULT_OK;
        if (ostate == PTHREAD_CANCEL_ENABLE) {
          pthread_testcancel();
        }
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





lagopus_result_t
lagopus_cond_create(lagopus_cond_t *cndptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_cond_t cnd = NULL;

  if (cndptr != NULL) {
    *cndptr = NULL;
    cnd = (lagopus_cond_t)malloc(sizeof(*cnd));
    if (cnd != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_cond_init(&(cnd->m_cond), NULL)) == 0) {
        cnd->m_creator_pid = getpid();
        *cndptr = cnd;
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
    if (ret != LAGOPUS_RESULT_OK) {
      free((void *)cnd);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_cond_destroy(lagopus_cond_t *cndptr) {
  if (cndptr != NULL &&
      *cndptr != NULL) {
    if ((*cndptr)->m_creator_pid == getpid()) {
      (void)pthread_cond_destroy(&((*cndptr)->m_cond));
    }
    free((void *)*cndptr);
    *cndptr = NULL;
  }
}


lagopus_result_t
lagopus_cond_wait(lagopus_cond_t *cndptr,
                  lagopus_mutex_t *mtxptr,
                  lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL &&
      cndptr != NULL &&
      *cndptr != NULL) {
    /*
     * It's kinda risky but we allow to cond-wait with recursive lock.
     *
     * So this condition is not checked:
     * (*mtxptr)->m_type != LAGOPUS_MUTEX_TYPE_RECURSIVE)
     */
    int st;

    errno = 0;
    if (nsec < 0) {
      if ((st = pthread_cond_wait(&((*cndptr)->m_cond),
                                  &((*mtxptr)->m_mtx))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      lagopus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);
    retry:
      errno = 0;
      if ((st = pthread_cond_timedwait(&((*cndptr)->m_cond),
                                       &((*mtxptr)->m_mtx),
                                       &ts)) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        if (st == EINTR) {
          goto retry;
        } else if (st == ETIMEDOUT) {
          ret = LAGOPUS_RESULT_TIMEDOUT;
        } else {
          errno = st;
          ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_cond_notify(lagopus_cond_t *cndptr,
                    bool for_all) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (cndptr != NULL &&
      *cndptr != NULL) {
    int st;

    /*
     * I know I don't need this but:
     */
    mbar();

    errno = 0;
    if ((st = ((for_all == true) ? s_notify_all_proc : s_notify_single_proc)(
                &((*cndptr)->m_cond))) == 0) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





lagopus_result_t
lagopus_barrier_create(lagopus_barrier_t *bptr, size_t n) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_barrier_t b = NULL;

  if (bptr != NULL && n > 0) {
    *bptr = NULL;
    b = (lagopus_barrier_t)malloc(sizeof(*b));
    if (b != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_barrier_init(&(b->m_barrier), NULL,
                                     (unsigned)n)) == 0) {
        b->m_creator_pid = getpid();
        *bptr = b;
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
    if (ret != LAGOPUS_RESULT_OK) {
      free((void *)b);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_barrier_destroy(lagopus_barrier_t *bptr) {
  if (bptr != NULL &&
      *bptr != NULL) {
    if ((*bptr)->m_creator_pid == getpid()) {
      (void)pthread_barrier_destroy(&((*bptr)->m_barrier));
    }
    free((void *)*bptr);
    *bptr = NULL;
  }
}


lagopus_result_t
lagopus_barrier_wait(lagopus_barrier_t *bptr, bool *is_master) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (bptr != NULL &&
      *bptr != NULL) {
    lagopus_barrier_t b = *bptr;
    int st;

    errno = 0;
    st = pthread_barrier_wait(&(b->m_barrier));
    if (st == 0 || st == PTHREAD_BARRIER_SERIAL_THREAD) {
      ret = LAGOPUS_RESULT_OK;
      if (st == PTHREAD_BARRIER_SERIAL_THREAD && is_master != NULL) {
        *is_master = true;
      }
    } else {
      errno = st;
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


