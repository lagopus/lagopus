#include "lagopus_apis.h"
#include "lagopus_pipeline_stage_internal.h"





typedef struct {
  lagopus_pipeline_stage_record s_stg;

  lagopus_hashmap_t *s_tblptr[2];
  lagopus_bbq_t *s_qptr;
} test_stage_record;
typedef test_stage_record 	*test_stage_t;


typedef struct {
  uint64_t s_val;
  lagopus_chrono_t s_begin_time;
  lagopus_chrono_t s_end_time;
} test_event_record;
typedef test_event_record 	*test_event_t;





static void
s_pre_pause(const lagopus_pipeline_stage_t *sptr) {
  (void)sptr;

  lagopus_msg_debug(1, "called.\n");
}


static lagopus_result_t
s_setup(const lagopus_pipeline_stage_t *sptr) {
  (void)sptr;

  lagopus_msg_debug(1, "called.\n");

  return LAGOPUS_RESULT_OK;
}


static lagopus_result_t
s_fetch(const lagopus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t max) {
  (void)sptr;
  (void)buf;
  (void)max;

  lagopus_msg_debug(1, "called " PFSZ(u) ".\n", idx);

  return (lagopus_result_t)max;
}


static lagopus_result_t
s_main(const lagopus_pipeline_stage_t *sptr,
       size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)buf;

  lagopus_msg_debug(1, "called " PFSZ(u) ".\n", idx);

  return (lagopus_result_t)n;
}


static lagopus_result_t
s_throw(const lagopus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)buf;

  lagopus_msg_debug(1, "called " PFSZ(u) ".\n", idx);

  return (lagopus_result_t)n;
}


static lagopus_result_t
s_sched(const lagopus_pipeline_stage_t *sptr,
        void *buf, size_t n) {
  (void)sptr;
  (void)buf;

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

#if 0
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

  return (st == LAGOPUS_RESULT_OK) ? 0 : 1;
}
