#include "lagopus_apis.h"

#include "base_stage.h"
#include "test_stage.h"





#include "base_stage.c"
#include "test_stage.c"





static inline lagopus_result_t
s_pipeline_create(test_stage_t *stages,
                  size_t max_stage,
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
                   LAGOPUS_RESULT_OK)) {
        goto done;
      }
    }

    ret = s_test_stage_create_by_spec(&(stages[max_stage - 1]),
                                      max_stage - 1, max_stage,
                                      egress_info);
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
 * str := worker % q % batch % sched % fetch
*/
static inline lagopus_result_t
s_set_stage_spec(test_stage_spec_t *spec, const char *str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(spec != NULL &&
             IS_VALID_STRING(str) == true)) {
    char *cstr = strdup(str);

    if (IS_VALID_STRING(cstr) == true) {
      char *tokens[5];

      if ((lagopus_str_tokenize_with_limit(cstr, tokens, 5, 5, ":|")) == 5) {
        size_t n_workers = 0;
        size_t n_qs = 0;
        size_t batch = 0;
        base_stage_sched_t sched = s_str2sched(tokens[3]);
        base_stage_fetch_t fetch = s_str2fetch(tokens[4]);

        if ((ret = lagopus_str_parse_uint64(tokens[0], &n_workers)) ==
            LAGOPUS_RESULT_OK &&
            (ret = lagopus_str_parse_uint64(tokens[1], &n_qs)) ==
            LAGOPUS_RESULT_OK &&
            (ret = lagopus_str_parse_uint64(tokens[2], &batch)) ==
            LAGOPUS_RESULT_OK) {
          spec->m_n_workers = n_workers;
          spec->m_n_qs = n_qs;
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





static size_t s_n_events = 1000 * 1000 * 1000;
static lagopus_chrono_t s_to = 100 * 1000;	/* 100 us. */

static test_stage_t *s_stages;
static size_t s_n_stages = 2;
static size_t s_n_ing_workers = 1;
static const char *s_int_spec_str = "1:1:1000:single:single";
static const char *s_egr_spec_str = "1:1:1000:single:single";

static test_stage_spec_t s_ingres_spec;
static test_stage_spec_t s_interm_spec;
static test_stage_spec_t s_egress_spec;


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
            LAGOPUS_RESULT_OK && tmp > 0) {
          s_n_events = utmp;
        } else {
          lagopus_msg_error("Invalid event size '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("An event size is not specified.\n");
        goto done;
      }
    } else if (strcasecmp(*argv, "-ns") == 0) {
      argv++;
      if (IS_VALID_STRING(*argv) == true) {
        utmp = 0;
        if ((ret = lagopus_str_parse_uint64(*argv, &utmp)) == 
            LAGOPUS_RESULT_OK && tmp > 0) {
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
            LAGOPUS_RESULT_OK && tmp > 0) {
          s_n_ing_workers = utmp;
        } else {
          lagopus_msg_error("Invalid ingress worker # '%s'.\n", *argv);
          goto done;
        }
      } else {
        lagopus_msg_error("An ingress worker # is not specified.\n");
        goto done;
      }
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

  ret = s_pipeline_create(s_stages, s_n_stages,
                          &s_ingres_spec,
                          &s_interm_spec,
                          &s_egress_spec);

done:
  return ret;
}





int
main(int argc, const char *const argv[]) {
  lagopus_result_t st = LAGOPUS_RESULT_ANY_FAILURES;

  st = s_parse_args(argc - 1, argv + 1);
  if (st != LAGOPUS_RESULT_OK) {
    goto done;
  }

  st = s_create();

#if 0
  lagopus_pipeline_stage_t s = NULL;

  size_t nthd = 1;

  (void)argc;

  if (IS_VALID_STRING(argv[1]) == true) {
    size_t tmp;
    if (lagopus_str_parse_uint64(argv[1], &tmp) == LAGOPUS_RESULT_OK &&
        tmp > 0LL) {
      nthd = tmp;
    }
  }

  fprintf(stdout, "Creating... ");
  st = lagopus_pipeline_stage_create(&s, 0, "a_test",
                                     nthd,
                                     sizeof(void *), 1024,
                                     s_pre_pause,
                                     s_sched,
                                     s_setup,
                                     s_fetch,
                                     s_main,
                                     s_throw,
                                     s_shutdown,
                                     s_finalize,
                                     s_freeup);
  if (st == LAGOPUS_RESULT_OK) {
    fprintf(stdout, "Created.\n");
    fprintf(stdout, "Setting up... ");
    st = lagopus_pipeline_stage_setup(&s);
    if (st == LAGOPUS_RESULT_OK) {
      fprintf(stdout, "Set up.\n");
      fprintf(stdout, "Starting... ");
      st = lagopus_pipeline_stage_start(&s);
      if (st == LAGOPUS_RESULT_OK) {
        fprintf(stdout, "Started.\n");
        fprintf(stdout, "Opening the front door... ");
        st = global_state_set(GLOBAL_STATE_STARTED);
        if (st == LAGOPUS_RESULT_OK) {
          char buf[1024];
          char *cmd = NULL;
          fprintf(stdout, "The front door is open.\n");

          fprintf(stdout, "> ");
          while (fgets(buf, sizeof(buf), stdin) != NULL &&
                 st == LAGOPUS_RESULT_OK) {
            (void)lagopus_str_trim_right(buf, "\r\n\t ", &cmd);

            if (strcasecmp(cmd, "pause") == 0 ||
                strcasecmp(cmd, "spause") == 0) {
              fprintf(stdout, "Pausing... ");
              if ((st = lagopus_pipeline_stage_pause(&s, -1LL)) ==
                  LAGOPUS_RESULT_OK) {
                if (strcasecmp(cmd, "spause") == 0) {
                  s_set(0LL);
                }
                fprintf(stdout, "Paused " PF64(u) "\n", s_get());
              } else {
                fprintf(stdout, "Failure.\n");
              }
            } else if (strcasecmp(cmd, "resume") == 0) {
              fprintf(stdout, "Resuming... ");
              if ((st = lagopus_pipeline_stage_resume(&s)) ==
                  LAGOPUS_RESULT_OK) {
                fprintf(stdout, "Resumed.\n");
              } else {
                fprintf(stdout, "Failure.\n");
              }
            } else if (strcasecmp(cmd, "get") == 0) {
              fprintf(stdout, PF64(u) "\n", s_get());
            }

            free((void *)cmd);
            cmd = NULL;
            fprintf(stdout, "> ");
          }
          fprintf(stdout, "\nDone.\n");

          fprintf(stdout, "Shutting down... ");
          st = lagopus_pipeline_stage_shutdown(&s, SHUTDOWN_GRACEFULLY);
          if (st == LAGOPUS_RESULT_OK) {
            fprintf(stdout, "Shutdown accepted... ");
            sleep(1);
            s_do_stop = true;
            fprintf(stdout, "Waiting shutdown... ");
            st = lagopus_pipeline_stage_wait(&s, -1LL);
            if (st == LAGOPUS_RESULT_OK) {
              fprintf(stdout, "OK, Shutdown.\n");
            }
          }
        }
      }
    }
  }
  fflush(stdout);

  if (st != LAGOPUS_RESULT_OK) {
    lagopus_perror(st);
  }

  fprintf(stdout, "Destroying... ");
  lagopus_pipeline_stage_destroy(&s);
  fprintf(stdout, "Destroyed.\n");

#endif

done:
  if (st != LAGOPUS_RESULT_OK) {
    lagopus_perror(st);
  }

  return (st == LAGOPUS_RESULT_OK) ? 0 : 1;
}
