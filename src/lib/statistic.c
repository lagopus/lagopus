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

#include <math.h>





typedef struct lagopus_statistic_struct {
  const char *m_name;
  volatile size_t m_n;
  volatile int64_t m_min;
  volatile int64_t m_max;
  volatile int64_t m_sum;
  volatile int64_t m_sum2;
} lagopus_statistic_struct;





static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static lagopus_hashmap_t s_stat_tbl;





static void s_ctors(void) __attr_constructor__(112);
static void s_dtors(void) __attr_destructor__(112);

static void s_destroy_stat(lagopus_statistic_t s, bool delhash);
static void s_stat_freeup(void *arg);

static lagopus_result_t s_reset_stat(lagopus_statistic_t s);





static void
s_once_proc(void) {
  lagopus_result_t r;

  if ((r = lagopus_hashmap_create(&s_stat_tbl,
                                  LAGOPUS_HASHMAP_TYPE_STRING,
                                  s_stat_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the stattistics table.\n");
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(10, "The statistics module is initialized.\n");
}


static inline void
s_final(void) {
  lagopus_hashmap_destroy(&s_stat_tbl, true);
}


static void
s_dtors(void) {
  if (lagopus_module_is_unloading() &&
      lagopus_module_is_finalized_cleanly()) {
    s_final();

    lagopus_msg_debug(10, "The stattitics module is finalized.\n");
  } else {
    lagopus_msg_debug(10, "The stattitics module is not finalized "
                      "because of module finalization problem.\n");
  }
}





static void
s_stat_freeup(void *arg) {
  if (likely(arg != NULL)) {
    lagopus_statistic_t s = (lagopus_statistic_t)arg;
    s_destroy_stat(s, false);
  }
}





static inline lagopus_result_t
s_create_stat(lagopus_statistic_t *sptr, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL &&
             IS_VALID_STRING(name) == true)) {
    lagopus_statistic_t s = (lagopus_statistic_t)malloc(sizeof(*s));
    const char *m_name = strdup(name);
    *sptr = NULL;

    if (likely(s != NULL && IS_VALID_STRING(m_name) == true)) {
      void *val = (void *)s;
      if (likely((ret = lagopus_hashmap_add(&s_stat_tbl, (void *)m_name,
                                            &val, false)) ==
                 LAGOPUS_RESULT_OK)) {
         s->m_name = m_name;
         s_reset_stat(s);
         *sptr = s;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    if (unlikely(ret != LAGOPUS_RESULT_OK)) {
      free((void *)s);
      free((void *)m_name);
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_destroy_stat(lagopus_statistic_t s, bool delhash) {
  if (likely(s != NULL)) {
    if (delhash == true) {
      (void)lagopus_hashmap_delete(&s_stat_tbl,
                                   (void *)s->m_name, NULL, false);
    }
    if (s->m_name != NULL) {
      free((void *)s->m_name);
    }
    free((void *)s);
  }
}


static inline lagopus_result_t
s_find_stat(lagopus_statistic_t *sptr, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL &&
             IS_VALID_STRING(name) == true)) {
    void *val = NULL;

    *sptr = NULL;

    ret = lagopus_hashmap_find(&s_stat_tbl, (void *)name, &val);
    if (likely(ret == LAGOPUS_RESULT_OK)) {
      *sptr = (lagopus_statistic_t)val;
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_reset_stat(lagopus_statistic_t s) {
  if (likely(s != NULL)) {
    s->m_n = 0LL;
    s->m_min = LLONG_MAX;
    s->m_max = LLONG_MIN;
    s->m_sum = 0LL;
    s->m_sum2 = 0LL;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


static inline lagopus_result_t
s_record_stat(lagopus_statistic_t s, int64_t val) {
  /*
   * TODO:
   *
   *	Well. should I use a mutex to provide the atomicity? I wonder
   *	four or more atomic operations costs more than takig a mutex
   *	lock or not.
   */

  if (likely(s != NULL)) {
    int64_t sum2;

    sum2 = val * val;

    (void)__sync_add_and_fetch(&(s->m_n), 1);
    (void)__sync_add_and_fetch(&(s->m_sum), val);
    (void)__sync_add_and_fetch(&(s->m_sum2), sum2);

    lagopus_atomic_update_min(int64_t, &(s->m_min), LLONG_MAX, val);
    lagopus_atomic_update_max(int64_t, &(s->m_max), LLONG_MIN, val);

    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


static inline lagopus_result_t
s_get_n_stat(lagopus_statistic_t s) {
  if (likely(s != NULL)) {
    return (lagopus_result_t)(__sync_fetch_and_add(&(s->m_n), 0));
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


static inline lagopus_result_t
s_get_min_stat(lagopus_statistic_t s, int64_t *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    *valptr = (__sync_fetch_and_add(&(s->m_min), 0));
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


static inline lagopus_result_t
s_get_max_stat(lagopus_statistic_t s, int64_t *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    *valptr = (__sync_fetch_and_add(&(s->m_max), 0));
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


static inline lagopus_result_t
s_get_avg_stat(lagopus_statistic_t s, double *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    int64_t n = (int64_t)__sync_fetch_and_add(&(s->m_n), 0); 

    if (n > 0) {
      double sum = (double)__sync_fetch_and_add(&(s->m_sum), 0);
      *valptr = sum / (double)n;
    } else {
      *valptr = 0.0;
    }

    return LAGOPUS_RESULT_OK;

  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


static inline lagopus_result_t
s_get_sd_stat(lagopus_statistic_t s, double *valptr, bool is_ssd) {
  if (likely(s != NULL && valptr != NULL)) {
    int64_t n = (int64_t)__sync_fetch_and_add(&(s->m_n), 0);

    if (n == 0) {
      *valptr = 0.0;
    } else {
      double sum = (double)__sync_fetch_and_add(&(s->m_sum), 0);
      double sum2 = (double)__sync_fetch_and_add(&(s->m_sum2), 0);
      double avg = sum / (double)n;
      double ssum = 
          sum2 -
          2.0 * avg * sum +
          avg * avg * (double)n;

      if (is_ssd == true) {
        if (n >= 2) {
          *valptr = sqrt((ssum / (double)(n - 1)));
        } else {
          *valptr = 0.0;
        }
      } else {
        if (n >= 1) {
          *valptr = sqrt(ssum / (double)n);
        } else {
          *valptr = 0.0;
        }
      }        
    }

    return LAGOPUS_RESULT_OK;

  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}





/*
 * Exported APIs
 */


lagopus_result_t
lagopus_statistic_create(lagopus_statistic_t *sptr, const char *name) {
  return s_create_stat(sptr, name);
}


lagopus_result_t
lagopus_statistic_find(lagopus_statistic_t *sptr, const char *name) {
  return s_find_stat(sptr, name);
}


void
lagopus_statistic_destroy(lagopus_statistic_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    s_destroy_stat(*sptr, true);
  }
}


void
lagopus_statistic_destroy_by_name(const char *name) {
  if (likely(IS_VALID_STRING(name) == true)) {
    lagopus_statistic_t s = NULL;
    if (likely(s_find_stat(&s, name) == LAGOPUS_RESULT_OK &&
               s != NULL)) {
      s_destroy_stat(s, true);
    }
  }
}


lagopus_result_t
lagopus_statistic_record(lagopus_statistic_t *sptr, int64_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_record_stat(*sptr, val);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_statistic_reset(lagopus_statistic_t *sptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_reset_stat(*sptr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_statistic_sample_n(lagopus_statistic_t *sptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_get_n_stat(*sptr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_statistic_min(lagopus_statistic_t *sptr, int64_t *valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_min_stat(*sptr, valptr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_statistic_max(lagopus_statistic_t *sptr, int64_t *valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_max_stat(*sptr, valptr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_statistic_average(lagopus_statistic_t *sptr, double *valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_avg_stat(*sptr, valptr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_statistic_sd(lagopus_statistic_t *sptr, double *valptr, bool is_ssd) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_sd_stat(*sptr, valptr, is_ssd);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
