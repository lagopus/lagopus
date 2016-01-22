#include "lagopus_apis.h"

#include "base_stage.h"
#include "test_stage.h"





#include "base_stage.c"
#include "test_stage.c"





static inline lagopus_result_t
s_pipeline_create(test_stage_t *stages,
                  size_t max_stage,
                  size_t weight,
                  test_stage_spec_t *ingress_info,
                  test_stage_spec_t *intermediate_info,
                  test_stage_spec_t *egress_info) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(stages != NULL &&
             max_stage > 1 &&
             ingress_info != NULL &&
             ((max_stage > 2) ? 
              ((intermediate_info != NULL) ? true : false) : true) &&
             egress_info != NULL)) {
    size_t i;

    (void)memset((void *)stages, 0, sizeof(*stages) * max_stage);

    if (unlikely((ret = s_test_stage_create_by_spec(&(stages[0]), 0, max_stage,
                                                    ingress_info)) != 
                 LAGOPUS_RESULT_OK)) {
      goto done;
    }

    for (i = 1; i < (max_stage - 1); i++) {
      if (unlikely((ret =
                    s_test_stage_create_by_spec(&(stages[i]), i, max_stage,
                                                intermediate_info)) != 
                   LAGOPUS_RESULT_OK ||
                   (ret = s_test_stage_set_weight(&(stages[i]), weight)) !=
                   LAGOPUS_RESULT_OK)) {
        goto done;
      }
    }

    ret = s_test_stage_create_by_spec(&(stages[max_stage - 1]),
                                      max_stage - 1, max_stage,
                                      egress_info);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = s_test_stage_set_weight(&(stages[max_stage - 1]), weight);
    }

 done:
    if (unlikely(ret != LAGOPUS_RESULT_OK)) {
      for (i = 0; i < max_stage; i++) {
        lagopus_pipeline_stage_destroy((lagopus_pipeline_stage_t *)stages[i]);
        stages[i] = NULL;
      }
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_pipeline_destroy(test_stage_t *stages,
                   size_t max_stage) {
  if (likely(stages != NULL)) {
    size_t i;

    for (i = 0; i < max_stage; i++) {
      lagopus_pipeline_stage_destroy((lagopus_pipeline_stage_t *)&stages[i]);
      stages[i] = NULL;
    }
  }
}





static inline base_stage_sched_t
s_str2sched(const char *str) {
  base_stage_sched_t ret = base_stage_sched_unknown;

  if (IS_VALID_STRING(str) == true) {
    if (strcasecmp(str, "single") == 0) {
      ret = base_stage_sched_single;
    } else if (strcasecmp(str, "hint") == 0) {
      ret = base_stage_sched_hint;
    } else if (strcasecmp(str, "rr") == 0) {
      ret = base_stage_sched_rr;
    } else if (strcasecmp(str, "rr_para") == 0) {
      ret = base_stage_sched_rr_para;
    }
  }

  return ret;
}


static inline base_stage_fetch_t
s_str2fetch(const char *str) {
  base_stage_fetch_t ret = base_stage_fetch_unknown;

  if (IS_VALID_STRING(str) == true) {
    if (strcasecmp(str, "single") == 0) {
      ret = base_stage_fetch_single;
    } else if (strcasecmp(str, "hint") == 0) {
      ret = base_stage_fetch_hint;
    } else if (strcasecmp(str, "rr") == 0) {
      ret = base_stage_fetch_rr;
    }
  }

  return ret;
}


static inline lagopus_result_t
s_allowed_queue_types(size_t n_qs,
                      base_stage_sched_t sched, 
                      base_stage_fetch_t fetch) {
  lagopus_result_t ret = LAGOPUS_RESULT_NOT_ALLOWED;

  if (unlikely(n_qs == 0 ||
               sched == base_stage_sched_unknown ||
               fetch == base_stage_fetch_unknown)) {
    goto done;
  }

  if (unlikely(n_qs == 1 &&
               (sched != base_stage_sched_single ||
                fetch != base_stage_fetch_single))) {
    goto done;
  }

  ret = LAGOPUS_RESULT_OK;

done:
  return ret;
}


/*
 * % := "|" | ":"
 *
 * str := worker % q % qlen % batch % sched % fetch
*/
static inline lagopus_result_t
s_set_stage_spec(test_stage_spec_t *spec, const char *str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(spec != NULL &&
             IS_VALID_STRING(str) == true)) {
    char *cstr = strdup(str);

    if (IS_VALID_STRING(cstr) == true) {
      char *tokens[5];

      if ((lagopus_str_tokenize_with_limit(cstr, tokens, 6, 6, ":|")) == 6) {
        size_t n_workers = 0;
        size_t n_qs = 0;
	size_t q_len = 0;
        size_t batch = 0;
        base_stage_sched_t sched = s_str2sched(tokens[4]);
        base_stage_fetch_t fetch = s_str2fetch(tokens[5]);

        if ((ret = lagopus_str_parse_uint64(tokens[0], &n_workers)) ==
            LAGOPUS_RESULT_OK &&
            (ret = lagopus_str_parse_uint64(tokens[1], &n_qs)) ==
            LAGOPUS_RESULT_OK &&
            (ret = lagopus_str_parse_uint64(tokens[2], &q_len)) ==
            LAGOPUS_RESULT_OK &&
            (ret = lagopus_str_parse_uint64(tokens[3], &batch)) ==
            LAGOPUS_RESULT_OK) {
          spec->m_n_workers = n_workers;
          spec->m_n_qs = n_qs;
	  spec->m_q_len = q_len;
          spec->m_batch_size = batch;

          if ((ret = s_allowed_queue_types(spec->m_n_qs, sched, fetch)) ==
              LAGOPUS_RESULT_OK) {
            spec->m_sched_type = sched;
            spec->m_fetch_type = fetch;
          } else {
            if (spec->m_n_qs == 1) {
              spec->m_sched_type = base_stage_sched_single;
              spec->m_fetch_type = base_stage_fetch_single;
              ret = LAGOPUS_RESULT_OK;
            }
          }
        }

      } else {

        if (ret > 0) {
          ret = LAGOPUS_RESULT_TOO_LONG;
        }

      }

      free((void *)cstr);

    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static size_t s_n_try = 1;
static size_t s_n_events = 8 * 1000 * 1000;
static lagopus_chrono_t s_to = 100 * 1000;	/* 100 us. */

static uint64_t *s_start_clocks = NULL;

static test_stage_t *s_stages;
static size_t s_n_stages = 2;
static size_t s_n_ing_workers = 1;
static const char *s_int_spec_str = "1:1:1000:1000:single:single";
static const char *s_egr_spec_str = "1:1:1000:1000:single:single";
static size_t s_weight = 100;

static test_stage_spec_t s_ingres_spec;
static test_stage_spec_t s_interm_spec;
static test_stage_spec_t s_egress_spec;

static uint64_t s_check_sum = 0LL;

static bool s_do_ref = false;


static lagopus_statistic_t s_total_time = NULL;
static lagopus_statistic_t s_total_clock = NULL;
static lagopus_statistic_t s_tick = NULL;
static lagopus_statistic_t s_throughput = NULL;
static lagopus_statistic_t s_ev_time = NULL;
static lagopus_statistic_t s_actual_meps = NULL;
static lagopus_statistic_t *s_latencies = NULL;





static inline lagopus_result_t
s_init(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  s_start_clocks = (uint64_t *)malloc(sizeof(uint64_t) * s_n_stages);
  if (s_start_clocks == NULL) {
    ret = LAGOPUS_RESULT_NO_MEMORY;
    goto done;
  }

  s_latencies = (lagopus_statistic_t *)malloc(sizeof(lagopus_statistic_t) *
                                              s_n_stages);
  if (s_latencies != NULL) {
    size_t i;
    char buf[32];

    (void)memset((void *)s_latencies, 0,
                 sizeof(lagopus_statistic_t) * s_n_stages);
    for (i = 0; i < s_n_stages; i++) {
      snprintf(buf, 32, "latency[" PFSZ(u) "]", i);
      ret = lagopus_statistic_create(&s_latencies[i], buf);
      if (ret != LAGOPUS_RESULT_OK) {
        goto done;
      }
    }
  }

  if ((ret = lagopus_statistic_create(&s_total_time, "total-time")) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }
  if ((ret = lagopus_statistic_create(&s_total_clock, "total-clock")) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }
  if ((ret = lagopus_statistic_create(&s_tick, "tick")) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }
  if ((ret = lagopus_statistic_create(&s_throughput, "throughput")) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = lagopus_statistic_create(&s_ev_time, "net load time")) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }
  if ((ret = lagopus_statistic_create(&s_actual_meps,
                                      "net load throughput")) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  ret = LAGOPUS_RESULT_OK;

done:
  return ret;
}


static inline void
s_final(void) {
  free((void *)s_start_clocks);

  if (s_latencies != NULL) {
    size_t i;

    for (i = 0; i < s_n_stages; i++) {
      lagopus_statistic_destroy(&s_latencies[i]);
    }

    free((void *)s_latencies);
  }

  lagopus_statistic_destroy(&s_total_time);
  lagopus_statistic_destroy(&s_total_clock);
  lagopus_statistic_destroy(&s_tick);
  lagopus_statistic_destroy(&s_throughput);
  lagopus_statistic_destroy(&s_ev_time);
  lagopus_statistic_destroy(&s_actual_meps);
}





static inline lagopus_result_t
s_parse_args(int argc, const char * const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  uint64_t utmp;
  int64_t tmp;

  (void)argc;

  while (*argv != NULL) {

    if (strcasecmp(*argv, "-ne") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        utmp = 0;
        if ((ret = lagopus_str_parse_uint64(*argv, &utmp)) == 
            LAGOPUS_RESULT_OK && utmp > 0) {
          s_n_events = utmp;
        } else {
          lagopus_msg_error("Invalid event size '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("An event size is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-n") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        utmp = 0;
        if ((ret = lagopus_str_parse_uint64(*argv, &utmp)) == 
            LAGOPUS_RESULT_OK && utmp > 0) {
          s_n_try = utmp;
        } else {
          lagopus_msg_error("Invalid test run # '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("A test run # is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-ns") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        utmp = 0;
        if ((ret = lagopus_str_parse_uint64(*argv, &utmp)) == 
            LAGOPUS_RESULT_OK && utmp > 0) {
          if (utmp >= 2) {
            s_n_stages = utmp;
          } else {
            lagopus_msg_error("The stage # must be >= 2.\n");
            ret = LAGOPUS_RESULT_TOO_SMALL;
            goto done;
          }
        } else {
          lagopus_msg_error("Invalid stage # '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("A stage # is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-w") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        utmp = 0;
        if ((ret = lagopus_str_parse_uint64(*argv, &utmp)) == 
            LAGOPUS_RESULT_OK && utmp > 0) {
          s_weight = utmp;
        } else {
          lagopus_msg_error("Invalid weight # '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("A weight # is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-to") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        tmp = 0;
        if ((ret = lagopus_str_parse_int64(*argv, &tmp)) == 
            LAGOPUS_RESULT_OK && tmp > 0) {
          s_to = tmp;
        } else {
          lagopus_msg_error("Invalid timeout '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("A timeout is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-int-spec") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        s_int_spec_str = *argv;
      } else {
        lagopus_msg_error("Intermediate stages spec. in not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-egr-spec") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        s_egr_spec_str = *argv;
      } else {
        lagopus_msg_error("An egress stage spec. is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-ing-wkrs") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        utmp = 0;
        if ((ret = lagopus_str_parse_uint64(*argv, &utmp)) == 
            LAGOPUS_RESULT_OK && utmp > 0) {
          s_n_ing_workers = utmp;
        } else {
          lagopus_msg_error("Invalid ingress worker # '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("An ingress worker # is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-ref") == 0) {
      s_do_ref = true;
    }

    argv++;
  }

  if (s_n_events > 0 &&
      s_n_stages >= 2 &&
      s_n_ing_workers > 0 &&
      IS_VALID_STRING(s_int_spec_str) == true &&
      IS_VALID_STRING(s_int_spec_str) == true) {

    s_ingres_spec.m_n_workers = s_n_ing_workers;
    s_ingres_spec.m_n_events = s_n_events;

    s_interm_spec.m_n_events = s_n_events;
    s_interm_spec.m_to = s_to;

    s_egress_spec.m_n_events = s_n_events;
    s_egress_spec.m_to = s_to;

    s_check_sum = (s_n_events + 1) * s_n_events / 2LL - s_n_events;

    if ((ret = s_set_stage_spec(&s_interm_spec, s_int_spec_str)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_msg_error("Invalid intermidiate stages spec. '%s'.\n",
                        s_int_spec_str);
      goto done;
    }
    if ((ret = s_set_stage_spec(&s_egress_spec, s_egr_spec_str)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_msg_error("Invalid egres stage spec. '%s'.\n",
                        s_int_spec_str);
      goto done;
    }

    ret = LAGOPUS_RESULT_OK;
  }

done:
  return ret;
}


static inline lagopus_result_t
s_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  s_stages = (test_stage_t *)malloc(sizeof(test_stage_t) * s_n_stages);
  if (s_stages == NULL) {
    ret = LAGOPUS_RESULT_NO_MEMORY;
    lagopus_msg_error("Can't allocate an array of stage.\n");
    goto done;
  }
  (void)memset((void *)s_stages, 0, sizeof(test_stage_t *) * s_n_stages);

  ret = s_pipeline_create(s_stages, s_n_stages, s_weight,
                          &s_ingres_spec,
                          &s_interm_spec,
                          &s_egress_spec);

done:
  return ret;
}


static inline lagopus_result_t
s_record(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  for (i = 1; i < s_n_stages; i++) {
    const char *name = NULL;
    (void)lagopus_pipeline_stage_get_name(
        (lagopus_pipeline_stage_t *)&(s_stages[i]),
        &name);
    if (s_stages[i]->m_n_events != s_n_events) {
      lagopus_msg_error("stage " PFSZ(u) " '%s' events # error, "
                        PFSZ(u) " != " PFSZ(u) "\n",
                        i, name,
                        s_stages[i]->m_n_events, s_n_events);
      goto done;
    }
    if (s_stages[i]->m_sum != s_check_sum) {
      fprintf(stderr, "\n");
      lagopus_msg_error("stage " PFSZ(u) " '%s' checksum error, "
                        PFSZ(u) " != " PFSZ(u) "\n",
                        i, name,
                        s_stages[i]->m_sum, s_check_sum);
      goto done;
    }
  }

  ret = LAGOPUS_RESULT_OK;

  if (ret == LAGOPUS_RESULT_OK) {
    uint64_t dummy0 = 0;
    uint64_t start_clock = 0;
    uint64_t end_clock = 0;
    lagopus_chrono_t dummy1 = 0;
    lagopus_chrono_t start_time = 0;
    lagopus_chrono_t end_time = 0;
    lagopus_chrono_t total_time = 0;
    uint64_t total_clock = 0;
    double tick = 0.0;
    double meps = 0.0;

    (void)s_test_stage_get_times(&s_stages[0],
                                 &start_time, &dummy1);
    (void)s_test_stage_get_times(&s_stages[s_n_stages - 1],
                                 &dummy1, &end_time);

    (void)s_test_stage_get_clocks(&s_stages[0],
                                  &start_clock, &dummy0);
    (void)s_test_stage_get_clocks(&s_stages[s_n_stages - 1],
                                  &dummy0, &end_clock);

    total_time = end_time - start_time;
    total_clock = end_clock - start_clock;
    tick = (double)total_time / (double)total_clock * 1000000.0;
    meps = (double)s_n_events / (double)total_time * 1000000.0;

    (void)lagopus_statistic_record(&s_total_time, (int64_t)total_time);
    (void)lagopus_statistic_record(&s_total_clock, (int64_t)total_clock);
    (void)lagopus_statistic_record(&s_tick, (int64_t)tick);
    (void)lagopus_statistic_record(&s_throughput, (int64_t)meps);

    for (i = 0; i < s_n_stages; i++) {
      (void)s_test_stage_get_clocks(&s_stages[i], &s_start_clocks[i], &dummy0);
    }

    for (i = 0; i < s_n_stages - 1; i++) {
      dummy0 = s_start_clocks[i + 1] - s_start_clocks[i];
      (void)lagopus_statistic_record(&s_latencies[i + 1], (int64_t)dummy0);
    }
  }

done:
  return ret;
}


static inline lagopus_result_t
s_run(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (s_stages != NULL) {
    size_t i;
    lagopus_pipeline_stage_t *s;

    for (i = 0; i < s_n_stages; i++) {
      s = (lagopus_pipeline_stage_t *)(&s_stages[i]);
      ret = lagopus_pipeline_stage_setup(s);
      if (ret != LAGOPUS_RESULT_OK) {
        break;
      }
    }

    if (ret == LAGOPUS_RESULT_OK) {
      /*
       * Start all the stages.
       */
      for (i = 0; i < s_n_stages; i++) {
        s = (lagopus_pipeline_stage_t *)(&s_stages[i]);
        ret = lagopus_pipeline_stage_start(s);
        if (ret != LAGOPUS_RESULT_OK) {
          break;
        }
      }
    }

    if (ret == LAGOPUS_RESULT_OK) {
      /*
       * Go.
       */
      ret = global_state_set(GLOBAL_STATE_STARTED);
      if (ret == LAGOPUS_RESULT_OK) {
        /*
         * Wait the last stage finish.
         */
        ret = s_test_stage_wait(s_stages[s_n_stages - 1]);
        if (ret == LAGOPUS_RESULT_OK) {
          /*
           * Send all the stages a shutdown notification.
           */
          for (i = 0; i < s_n_stages; i++) {
            s = (lagopus_pipeline_stage_t *)(&s_stages[i]);
            ret = lagopus_pipeline_stage_shutdown(s, SHUTDOWN_GRACEFULLY);
            if (ret != LAGOPUS_RESULT_OK) {
              break;
            }
          }
          if (ret == LAGOPUS_RESULT_OK) {
            /*
             * Wake all the stages up, then all the stage workers exit.
             */
            for (i = 0; i < s_n_stages; i++) {
              ret = s_test_stage_wakeup(s_stages[i]);
              if (ret != LAGOPUS_RESULT_OK) {
                break;
              }
            }
            if (ret == LAGOPUS_RESULT_OK) {
              /*
               * Wait all the stages finish.
               */
              for (i = 0; i < s_n_stages; i++) {
                s = (lagopus_pipeline_stage_t *)(&s_stages[i]);
                ret = lagopus_pipeline_stage_wait(s, -1LL);
                if (ret != LAGOPUS_RESULT_OK) {
                  break;
                }
              }
            }
          }
        }
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_ref(size_t n_try) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t n = s_n_events;
  uint64_t *dst = (uint64_t *)malloc(sizeof(uint64_t) * n);
  uint64_t *src = (uint64_t *)malloc(sizeof(uint64_t) * n);

  if (dst != NULL && src != NULL) {
    size_t sum = 0;
    size_t i;
    size_t j;
    size_t k;
    lagopus_chrono_t start = 0;
    lagopus_chrono_t ev_start = 0;
    lagopus_chrono_t end = 0;

    for (i = 0; i < s_n_events; i++) {
      src[i] = i;
    }

    fprintf(stderr, "\n");

    for (i = 0; i < n_try; i++) {

      fprintf(stderr, "iter " PFSZ(u) " ... ", i + 1);

      sum = 0;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(start);

      (void)memcpy((void *)dst, (void *)src, sizeof(uint64_t) * n);

      WHAT_TIME_IS_IT_NOW_IN_NSEC(ev_start);

      for (j = 0; j < s_weight; j++) {
        sum = 0;
        for (k = 0; k < n; k++) {
          sum += dst[k];
        }
      }

      WHAT_TIME_IS_IT_NOW_IN_NSEC(end);

      if (sum == s_check_sum) {
        lagopus_chrono_t total_time = end - start;
        lagopus_chrono_t ev_time = end - ev_start;
        double meps = (double)s_n_events / (double)total_time * 1000000.0;
        double actual_meps = (double)s_n_events / (double)ev_time * 1000000.0;

        (void)lagopus_statistic_record(&s_total_time, (int64_t)total_time);
        (void)lagopus_statistic_record(&s_ev_time, (int64_t)ev_time);
        (void)lagopus_statistic_record(&s_throughput, (int64_t)meps);
        (void)lagopus_statistic_record(&s_actual_meps, (int64_t)actual_meps);

        fprintf(stderr, "done.\r");
      } else {
        fprintf(stderr, "checksum error.\n");
        goto done;
      }
    }

    ret = LAGOPUS_RESULT_OK;

 done:
    free((void *)src);
    free((void *)dst);
  }

  fprintf(stderr, "\n");

  return ret;
}


static inline void
s_destroy(void) {
  s_pipeline_destroy(s_stages, s_n_stages);
}





static inline lagopus_result_t
s_try(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = s_create();
  if (ret == LAGOPUS_RESULT_OK) {
    ret = s_run();
    if (ret == LAGOPUS_RESULT_OK) {
      ret = s_record();
    }
    s_destroy();
  }

  return ret;
}


static inline lagopus_result_t
s_iterate(size_t n) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  fprintf(stderr, "\n");

  for (i = 0; i < n; i++) {
    fprintf(stderr, "iter " PFSZ(u) " ... ", i + 1);
    ret = s_try();
    if (ret != LAGOPUS_RESULT_OK) {
      break;
    } else {
      fprintf(stderr, "done.\r");
    }
  }

  fprintf(stderr, "\n");

  return ret;
}





static inline void
s_print_statistic(FILE *fd, lagopus_statistic_t *s,
                  const char *title, const char *fmt, const char *unit,
                  double scale) {
  char buf[1024];

  int64_t min = 0;
  int64_t max = 0;
  double avg = 0.0;
  double sd = 0.0;
  double dmin = 0.0;
  double dmax = 0.0;

  (void)lagopus_statistic_min(s, &min);
  (void)lagopus_statistic_max(s, &max);
  (void)lagopus_statistic_average(s, &avg);
  (void)lagopus_statistic_sd(s, &sd, false);

  dmin = (double)min * scale;
  dmax = (double)max * scale;
  avg = avg * scale;
  sd = sd * scale;

  snprintf(buf, sizeof(buf),
           "%s (min/avg/max/sd):\n\t%s / %s / %s / %s %s\n",
           title, fmt, fmt, fmt, fmt, unit);
  fprintf(fd, (const char *)buf, dmin, avg, dmax, sd);

  fflush(fd);
}


static inline void
s_result(void) {
  size_t i;
  char buf[32];
  double tick = 0.0;

  fprintf(stdout, "\nResult:\n");

  if (s_do_ref == false) {
    s_print_statistic(stdout, &s_total_time,
                      "\tTotal time", "%12.4f", "msec.", 1.0/1000000.0);
    s_print_statistic(stdout, &s_total_clock,
                      "\tTotal clock", "%16.4f", "clock.", 1.0);
    s_print_statistic(stdout, &s_tick,
                      "\tTick", "%f", "nsec.", 1.0/1000000.0);
    s_print_statistic(stdout, &s_throughput,
                      "\tThroughput", "%16.4f", "M events/sec.", 1.0/1000.0);

    (void)lagopus_statistic_average(&s_tick, &tick);
    tick /= 1000000.0;

    for (i = 1; i < s_n_stages; i++) {
      snprintf(buf, sizeof(buf), "\tLatency[" PFSZS(2, u) "]", i);
      s_print_statistic(stdout, &s_latencies[i],
                        buf, "%10.2f", "nsec.", tick);
    }
  } else {
    s_print_statistic(stdout, &s_total_time,
                      "\tTotal time", "%12.4f", "msec.", 1.0/1000000.0);
    s_print_statistic(stdout, &s_ev_time,
                      "\tTotal load time", "%12.4f", "msec.", 1.0/1000000.0);
    s_print_statistic(stdout, &s_throughput,
                      "\tThroughput", "%16.4f", "M events/sec.", 1.0/1000.0);
    s_print_statistic(stdout, &s_actual_meps,
                      "\tLoad throughput", "%16.4f", "M events/sec.",
                      1.0/1000.0);
  }
}





int
main(int argc, const char *const argv[]) {
  lagopus_result_t st = LAGOPUS_RESULT_ANY_FAILURES;

  st = s_parse_args(argc - 1, argv + 1);
  if (st != LAGOPUS_RESULT_OK) {
    goto done;
  }

  st = s_init();
  if (st == LAGOPUS_RESULT_OK) {
    if (s_do_ref == false) {
      if (st != LAGOPUS_RESULT_OK) {
        goto done;
      }

      st = s_iterate(s_n_try);
    } else {
      st = s_ref(s_n_try);
    }
    if (st == LAGOPUS_RESULT_OK) {
      s_result();
    }
  }

done:
  s_final();

  if (st != LAGOPUS_RESULT_OK) {
    lagopus_perror(st);
  }

  return (st == LAGOPUS_RESULT_OK) ? 0 : 1;
}
