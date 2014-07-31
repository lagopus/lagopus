/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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





struct lagopus_mutex_record {
  pthread_mutex_t m_mtx;
  pid_t m_creator_pid;
  int m_prev_cancel_state;
};


struct lagopus_rwlock_record {
  pthread_rwlock_t m_rwl;
  pid_t m_creator_pid;
  int m_prev_cancel_state;
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





static notify_proc_t notify_single = pthread_cond_signal;
static notify_proc_t notify_all = pthread_cond_broadcast;





lagopus_result_t
lagopus_mutex_create(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_mutex_t mtx = NULL;

  if (mtxptr != NULL) {
    *mtxptr = NULL;
    mtx = (lagopus_mutex_t)malloc(sizeof(*mtx));
    if (mtx != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_mutex_init(&(mtx->m_mtx), NULL)) == 0) {
        mtx->m_creator_pid = getpid();
        mtx->m_prev_cancel_state = -INT_MAX;
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

    errno = 0;
    if ((st = pthread_mutex_init(&((*mtxptr)->m_mtx), NULL)) == 0) {
      (*mtxptr)->m_prev_cancel_state = -INT_MAX;
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
lagopus_mutex_lock(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
      (*mtxptr)->m_prev_cancel_state = -INT_MAX;
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
      (*mtxptr)->m_prev_cancel_state = -INT_MAX;
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
lagopus_mutex_timedlock(lagopus_mutex_t *mtxptr,
                        lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        (*mtxptr)->m_prev_cancel_state = -INT_MAX;
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
        (*mtxptr)->m_prev_cancel_state = -INT_MAX;
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
lagopus_mutex_unlock(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    /*
     * The caller must have this mutex locked.
     */
    if ((*mtxptr)->m_prev_cancel_state == -INT_MAX) {
      if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      /*
       * Someone (including the caller itself) locked the mutex with
       * lagopus_mutex_enter_critical() API.
       */
      ret = LAGOPUS_RESULT_CRITICAL_REGION_NOT_CLOSED;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_mutex_enter_critical(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    int oldstate;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     &oldstate)) == 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        (*mtxptr)->m_prev_cancel_state = oldstate;
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
lagopus_mutex_leave_critical(lagopus_mutex_t *mtxptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    int oldstate;

    /*
     * The caller must have this mutex locked.
     */
    oldstate = (*mtxptr)->m_prev_cancel_state;
    if (oldstate == PTHREAD_CANCEL_ENABLE ||
        oldstate == PTHREAD_CANCEL_DISABLE) {
      /*
       * This mutex is locked via lagopus_mutex_enter_critical().
       */
      if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
        if ((st = pthread_setcancelstate(oldstate, NULL)) == 0) {
          ret = LAGOPUS_RESULT_OK;
          pthread_testcancel();
        } else {
          errno = st;
          ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        }
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_CRITICAL_REGION_NOT_OPENED;
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
        rwl->m_prev_cancel_state = -INT_MAX;
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
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
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
    if ((*rwlptr)->m_prev_cancel_state == -INT_MAX) {
      if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      /*
       * Someone (including the caller itself) locked the rwlock with
       * lagopus_rwlock_enter_critical() API.
       */
      ret = LAGOPUS_RESULT_CRITICAL_REGION_NOT_CLOSED;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_rwlock_reader_enter_critical(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    int oldstate;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     &oldstate)) == 0) {
      if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
        (*rwlptr)->m_prev_cancel_state = oldstate;
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
lagopus_rwlock_writer_enter_critical(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    int oldstate;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     &oldstate)) == 0) {
      if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
        (*rwlptr)->m_prev_cancel_state = oldstate;
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
lagopus_rwlock_leave_critical(lagopus_rwlock_t *rwlptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    int oldstate;

    /*
     * The caller must have this rwlock locked.
     */
    oldstate = (*rwlptr)->m_prev_cancel_state;
    if (oldstate == PTHREAD_CANCEL_ENABLE ||
        oldstate == PTHREAD_CANCEL_DISABLE) {
      /*
       * This rwlock is locked via lagopus_rwlock_enter_critical().
       */
      if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
        if ((st = pthread_setcancelstate(oldstate, NULL)) == 0) {
          ret = LAGOPUS_RESULT_OK;
          pthread_testcancel();
        } else {
          errno = st;
          ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        }
      } else {
        errno = st;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_CRITICAL_REGION_NOT_OPENED;
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

    errno = 0;
    if ((st = ((for_all == true) ? notify_all : notify_single)(
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


