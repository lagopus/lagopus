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





static volatile bool s_do_stop = false;

static lagopus_rwlock_t s_lock = NULL;
static volatile uint64_t s_sum = 0LL;





static inline void
s_incr(void) {
  (void)lagopus_rwlock_writer_lock(&s_lock);
  s_sum++;
  (void)lagopus_rwlock_unlock(&s_lock);
}


static inline uint64_t
s_get(void) {
  uint64_t ret;

  (void)lagopus_rwlock_reader_lock(&s_lock);
  ret = s_sum;
  (void)lagopus_rwlock_unlock(&s_lock);

  return ret;
}


static inline void
s_set(uint64_t v) {
  (void)lagopus_rwlock_writer_lock(&s_lock);
  s_sum = v;
  (void)lagopus_rwlock_unlock(&s_lock);
}





static void
s_pre_pause(const lagopus_pipeline_stage_t *sptr) {
  (void)sptr;

  lagopus_msg_debug(1, "called.\n");
}


static lagopus_result_t
s_setup(const lagopus_pipeline_stage_t *sptr) {
  (void)sptr;

  (void)lagopus_rwlock_create(&s_lock);
  lagopus_msg_debug(1, "called.\n");

  return LAGOPUS_RESULT_OK;
}


static lagopus_result_t
s_fetch(const lagopus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t max) {
  (void)sptr;
  (void)idx;
  (void)buf;
  (void)max;

  return (s_do_stop == false) ? 1LL : 0LL;
}


static lagopus_result_t
s_main(const lagopus_pipeline_stage_t *sptr,
       size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)buf;

  s_incr();

  lagopus_msg_debug(1, "called " PFSZ(u) ".\n", idx);

  return (lagopus_result_t)n;
}


static lagopus_result_t
s_throw(const lagopus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)idx;
  (void)buf;

  return (lagopus_result_t)n;
}


static lagopus_result_t
s_sched(const lagopus_pipeline_stage_t *sptr,
        void *buf, size_t n, void *hint) {
  (void)sptr;
  (void)buf;
  (void)hint;

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

  if (s_lock != NULL) {
    (void)lagopus_rwlock_destroy(&s_lock);
    s_lock = NULL;
  }
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

  return (st == LAGOPUS_RESULT_OK) ? 0 : 1;
}
