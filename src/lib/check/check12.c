#include "lagopus_apis.h"
#include "lagopus_pipeline_stage_internal.h"





typedef struct {
  lagopus_pipeline_stage_record m_stg;

  lagopus_mutex_t m_lock;

  size_t m_n_max_count;
  size_t m_mod;

  size_t *m_counts;
} test_stage_record;
typedef test_stage_record 	*test_stage_t;





static void
s_pre_pause(const lagopus_pipeline_stage_t *sptr) {
  (void)sptr;

  lagopus_msg_debug(1, "called.\n");
}


static lagopus_result_t
s_setup(const lagopus_pipeline_stage_t *sptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  test_stage_t t = (test_stage_t)(*sptr);

  lagopus_msg_debug(1, "called.\n");

  t->m_counts = (size_t *)malloc(sizeof(size_t) *
                                 t->m_stg.m_n_workers);
  if (t->m_counts != NULL &&
      (ret = lagopus_mutex_create(&(t->m_lock))) == LAGOPUS_RESULT_OK) {
    size_t i;

    for (i = 0; i < t->m_stg.m_n_workers; i++) {
      t->m_counts[i] = 0LL;
    }

    ret = LAGOPUS_RESULT_OK;
  } else {
    if (t->m_counts == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  }

  return ret;
}


static lagopus_result_t
s_fetch(const lagopus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t max) {
  test_stage_t t = (test_stage_t)(*sptr);

  (void)buf;
  (void)max;

  if (t->m_counts[idx] < t->m_n_max_count) {
    return (lagopus_result_t)max;
  } else {
    return 0LL;
  }
}


static lagopus_result_t
s_main(const lagopus_pipeline_stage_t *sptr,
       size_t idx, void *buf, size_t n) {
  test_stage_t t = (test_stage_t)(*sptr);

  (void)buf;

  if (t->m_counts[idx] < t->m_n_max_count) {

#if 0
    if (t->m_mod > 0 && (t->m_counts[idx] % t->m_mod) == 0) {
      (void)lagopus_mutex_lock(&(t->m_lock));
      fprintf(stderr, "idx " PFSZ(u) ": " PFSZ(u) "\n",
              idx, t->m_counts[idx]);
      (void)lagopus_mutex_unlock(&(t->m_lock));
    }
#endif

    t->m_counts[idx]++;

    return (lagopus_result_t)n;
  } else {
    (void)lagopus_mutex_lock(&(t->m_lock));
    fprintf(stderr, "idx " PFSZ(u) ": " PFSZ(u) "\n",
            idx, t->m_counts[idx]);
    (void)lagopus_mutex_unlock(&(t->m_lock));
    return 0LL;
  }
}


static lagopus_result_t
s_throw(const lagopus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t n) {
  test_stage_t t = (test_stage_t)(*sptr);

  (void)buf;

  if (t->m_counts[idx] < t->m_n_max_count) {
    return (lagopus_result_t)n;
  } else {
    return 0LL;
  }
}


static lagopus_result_t
s_sched(const lagopus_pipeline_stage_t *sptr,
        void *buf, size_t n, void *hint) {
  (void)sptr;
  (void)buf;
  (void)hint;

  lagopus_msg_debug(1, "called.\n");

  return (lagopus_result_t)n;
}


static lagopus_result_t
s_shutdown(const lagopus_pipeline_stage_t *sptr,
           shutdown_grace_level_t l) {
  (void)sptr;

  lagopus_msg_debug(1, "called with \"%s\".\n",
                    (l == SHUTDOWN_RIGHT_NOW) ? "right now" : "gracefully");

  return LAGOPUS_RESULT_OK;
}


static void
s_finalize(const lagopus_pipeline_stage_t *sptr,
           bool is_canceled) {
  (void)sptr;

  lagopus_msg_debug(1, "%s.\n",
                    (is_canceled == false) ? "exit normally" : "canceled");
}


static void
s_freeup(const lagopus_pipeline_stage_t *sptr) {
  (void)sptr;

  lagopus_msg_debug(1, "called.\n");
}





int
main(int argc, const char *const argv[]) {
  lagopus_result_t st = LAGOPUS_RESULT_ANY_FAILURES;
  test_stage_t t = NULL;
  size_t tmp;

  size_t nthd = 1;
  size_t c_max = 1000LL * 1000LL * 100LL;
  size_t mod = 0;

  lagopus_chrono_t t_begin;
  lagopus_chrono_t t_end;
  lagopus_chrono_t t_total;

  (void)argc;

  WHAT_TIME_IS_IT_NOW_IN_NSEC(t_begin);

  if (IS_VALID_STRING(argv[1]) == true) {
    if (lagopus_str_parse_uint64(argv[1], &tmp) == LAGOPUS_RESULT_OK &&
        tmp > 0LL) {
      nthd = tmp;
    }
    if (IS_VALID_STRING(argv[2]) == true) {
      if (lagopus_str_parse_uint64(argv[2], &tmp) == LAGOPUS_RESULT_OK &&
          tmp > 0LL) {
        c_max = tmp;
      }
      if (IS_VALID_STRING(argv[3]) == true) {
        if (lagopus_str_parse_uint64(argv[3], &tmp) == LAGOPUS_RESULT_OK) {
          mod = tmp;
        }
      }
    }
  }

  st = lagopus_pipeline_stage_create((lagopus_pipeline_stage_t *)&t,
                                     sizeof(*t), "a_test",
                                     nthd,
                                     sizeof(void *), 1024,
                                     s_pre_pause,
                                     s_sched,
                                     s_setup,
                                     /* s_fetch, */ NULL,
                                     s_main,
                                     /* s_throw, */ NULL,
                                     s_shutdown,
                                     s_finalize,
                                     s_freeup);
  if (st == LAGOPUS_RESULT_OK) {
    t->m_n_max_count = c_max;
    t->m_mod = mod;

    st = lagopus_pipeline_stage_setup((lagopus_pipeline_stage_t *)&t);
    if (st == LAGOPUS_RESULT_OK) {
      st = lagopus_pipeline_stage_start((lagopus_pipeline_stage_t *)&t);
      if (st == LAGOPUS_RESULT_OK) {
        WHAT_TIME_IS_IT_NOW_IN_NSEC(t_begin);
        st = global_state_set(GLOBAL_STATE_STARTED);
        if (st == LAGOPUS_RESULT_OK) {
          sleep(1);
          st = lagopus_pipeline_stage_shutdown((lagopus_pipeline_stage_t *)&t,
                                               SHUTDOWN_GRACEFULLY);
          if (st == LAGOPUS_RESULT_OK) {
            st = lagopus_pipeline_stage_wait((lagopus_pipeline_stage_t *)&t,
                                             -1LL);
          }
        }
      }
    }
  }

  WHAT_TIME_IS_IT_NOW_IN_NSEC(t_end);

  t_total = t_end - t_begin;

  if (st != LAGOPUS_RESULT_OK) {
    lagopus_perror(st);
  }

  fprintf(stdout,
          "total %f sec.\n\n"
          "single thd throughput: %f Mops/s\n"
          "total throuput:        %f Mops/s\n\n"

          "%f nsec/op\n",

          (double)t_total / 1000.0 / 1000.0 / 1000.0,
          (double)c_max / (double)t_total * 1000.0,
          (double)c_max * (double)nthd / (double)t_total * 1000.0,
          (double)t_total / (double)c_max);

  return (st == LAGOPUS_RESULT_OK) ? 0 : 1;
}
