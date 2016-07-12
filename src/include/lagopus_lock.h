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

#ifndef __LAGOPUS_LOCK_H__
#define __LAGOPUS_LOCK_H__





/**
 *	@file	lagopus_lock.h
 */





typedef enum {
  LAGOPUS_MUTEX_TYPE_UNKNOWN = 0,
  LAGOPUS_MUTEX_TYPE_DEFAULT,
  LAGOPUS_MUTEX_TYPE_RECURSIVE
} lagopus_mutex_type_t;


typedef struct lagopus_mutex_record 	*lagopus_mutex_t;
typedef struct lagopus_rwlock_record 	*lagopus_rwlock_t;

typedef struct lagopus_cond_record 	*lagopus_cond_t;

typedef struct lagopus_barrier_record 	*lagopus_barrier_t;





lagopus_result_t
lagopus_mutex_create(lagopus_mutex_t *mtxptr);

lagopus_result_t
lagopus_mutex_create_recursive(lagopus_mutex_t *mtxptr);

void
lagopus_mutex_destroy(lagopus_mutex_t *mtxptr);

lagopus_result_t
lagopus_mutex_reinitialize(lagopus_mutex_t *mtxptr);

lagopus_result_t
lagopus_mutex_get_type(lagopus_mutex_t *mtxptr, lagopus_mutex_type_t *tptr);

lagopus_result_t
lagopus_mutex_lock(lagopus_mutex_t *mtxptr);

lagopus_result_t
lagopus_mutex_trylock(lagopus_mutex_t *mtxptr);

lagopus_result_t
lagopus_mutex_timedlock(lagopus_mutex_t *mtxptr, lagopus_chrono_t nsec);


lagopus_result_t
lagopus_mutex_unlock(lagopus_mutex_t *mtxptr);


lagopus_result_t
lagopus_mutex_enter_critical(lagopus_mutex_t *mtxptr, int *ostateptr);

lagopus_result_t
lagopus_mutex_leave_critical(lagopus_mutex_t *mtxptr, int ostate);





lagopus_result_t
lagopus_rwlock_create(lagopus_rwlock_t *rwlptr);

void
lagopus_rwlock_destroy(lagopus_rwlock_t *rwlptr);

lagopus_result_t
lagopus_rwlock_reinitialize(lagopus_rwlock_t *rwlptr);


lagopus_result_t
lagopus_rwlock_reader_lock(lagopus_rwlock_t *rwlptr);

lagopus_result_t
lagopus_rwlock_reader_trylock(lagopus_rwlock_t *rwlptr);

lagopus_result_t
lagopus_rwlock_reader_timedlock(lagopus_rwlock_t *rwlptr,
                                lagopus_chrono_t nsec);

lagopus_result_t
lagopus_rwlock_writer_lock(lagopus_rwlock_t *rwlptr);

lagopus_result_t
lagopus_rwlock_writer_trylock(lagopus_rwlock_t *rwlptr);

lagopus_result_t
lagopus_rwlock_writer_timedlock(lagopus_rwlock_t *rwlptr,
                                lagopus_chrono_t nsec);

lagopus_result_t
lagopus_rwlock_unlock(lagopus_rwlock_t *rwlptr);


lagopus_result_t
lagopus_rwlock_reader_enter_critical(lagopus_rwlock_t *rwlptr, int *ostateptr);

lagopus_result_t
lagopus_rwlock_writer_enter_critical(lagopus_rwlock_t *rwlptr, int *ostateptr);

lagopus_result_t
lagopus_rwlock_leave_critical(lagopus_rwlock_t *rwlptr, int ostate);





lagopus_result_t
lagopus_cond_create(lagopus_cond_t *cndptr);

void
lagopus_cond_destroy(lagopus_cond_t *cndptr);

lagopus_result_t
lagopus_cond_wait(lagopus_cond_t *cndptr,
                  lagopus_mutex_t *mtxptr,
                  lagopus_chrono_t nsec);

lagopus_result_t
lagopus_cond_notify(lagopus_cond_t *cndptr,
                    bool for_all);





lagopus_result_t
lagopus_barrier_create(lagopus_barrier_t *bptr, size_t n);


void
lagopus_barrier_destroy(lagopus_barrier_t *bptr);


lagopus_result_t
lagopus_barrier_wait(lagopus_barrier_t *bptr, bool *is_master);





typedef pthread_spinlock_t lagopus_spinlock_t;


static inline lagopus_result_t
lagopus_spinlock_initialize(lagopus_spinlock_t *l) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_init(l, PTHREAD_PROCESS_PRIVATE)) == 0)) {
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


static inline lagopus_result_t
lagopus_spinlock_lock(lagopus_spinlock_t *l) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_lock(l) == 0))) {
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


static inline lagopus_result_t
lagopus_spinlock_trylock(lagopus_spinlock_t *l) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_trylock(l) == 0))) {
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


static inline lagopus_result_t
lagopus_spinlock_unlock(lagopus_spinlock_t *l) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_unlock(l) == 0))) {
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


static inline void
lagopus_spinlock_finalize(lagopus_spinlock_t *l) {
  if (likely(l != NULL)) {
    (void)pthread_spin_destroy(l);
  }
}


#endif /* __LAGOPUS_LOCK_H__ */
