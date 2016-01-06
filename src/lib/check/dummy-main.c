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





#define ONE_SEC		1000LL * 1000LL * 1000LL
#define REQ_TIMEDOUT	ONE_SEC





static volatile bool s_got_term_sig = false;


static void
s_term_handler(int sig) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  global_state_t gs = GLOBAL_STATE_UNKNOWN;

  if ((r = global_state_get(&gs)) == LAGOPUS_RESULT_OK) {

    if ((int)gs >= (int)GLOBAL_STATE_STARTED) {

      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;
      if (sig == SIGTERM || sig == SIGINT) {
        l = SHUTDOWN_GRACEFULLY;
      } else if (sig == SIGQUIT) {
        l = SHUTDOWN_RIGHT_NOW;
      }
      if (IS_VALID_SHUTDOWN(l) == true) {
        lagopus_msg_debug(5, "About to request shutdown(%s)...\n",
                          (l == SHUTDOWN_RIGHT_NOW) ?
                          "RIGHT_NOW" : "GRACEFULLY");
        if ((r = global_state_request_shutdown(l)) == LAGOPUS_RESULT_OK) {
          lagopus_msg_debug(5, "the shutdown request accepted.\n");
        } else {
          lagopus_perror(r);
          lagopus_msg_error("can't request shutdown.\n");
        }
      }

    } else {
      if (sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        s_got_term_sig = true;
      }
    }

  }

}


static void
s_hup_handler(int sig) {
  (void)sig;
  lagopus_msg_debug(5, "called. it's dummy for now.\n");
}


static lagopus_chrono_t s_to = -1LL;


static inline void
usage(FILE *fd) {
  fprintf(fd, "usage:\n");
  fprintf(fd, "\t--help\tshow this.\n");
  fprintf(fd, "\t-to #\tset shutdown timeout (in sec.).\n");
  lagopus_module_usage_all(fd);
  (void)fflush(fd);
}


static void
parse_args(int argc, const char *const argv[]) {
  (void)argc;

  while (*argv != NULL) {
    if (strcmp("--help", *argv) == 0) {
      usage(stderr);
      exit(0);
    } else if (strcmp("-to", *argv) == 0) {
      if (IS_VALID_STRING(*(argv + 1)) == true) {
        int64_t tmp;
        argv++;
        if (lagopus_str_parse_int64(*argv, &tmp) == LAGOPUS_RESULT_OK) {
          if (tmp >= 0) {
            s_to = ONE_SEC * tmp;
          }
        } else {
          fprintf(stderr, "can't parse \"%s\" as a number.\n", *argv);
          exit(1);
        }
      } else {
        fprintf(stderr, "A timeout # is not specified.\n");
        exit(1);
      }
    }
    argv++;
  }
}


int
main(int argc, const char *const argv[]) {
  lagopus_result_t st = LAGOPUS_RESULT_ANY_FAILURES;

  (void)lagopus_signal(SIGHUP, s_hup_handler, NULL);
  (void)lagopus_signal(SIGINT, s_term_handler, NULL);
  (void)lagopus_signal(SIGTERM, s_term_handler, NULL);
  (void)lagopus_signal(SIGQUIT, s_term_handler, NULL);

  parse_args(argc - 1, argv + 1);

  (void)global_state_set(GLOBAL_STATE_INITIALIZING);

  fprintf(stderr, "Initializing... ");
  if ((st = lagopus_module_initialize_all(argc, argv)) ==
      LAGOPUS_RESULT_OK &&
      s_got_term_sig == false) {
    fprintf(stderr, "Initialized.\n");
    fprintf(stderr, "Starting... ");
    (void)global_state_set(GLOBAL_STATE_STARTING);
    if ((st = lagopus_module_start_all()) ==
        LAGOPUS_RESULT_OK &&
        s_got_term_sig == false) {
      fprintf(stderr, "Started.\n");
      if ((st = global_state_set(GLOBAL_STATE_STARTED)) ==
          LAGOPUS_RESULT_OK) {

        shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;

        fprintf(stderr, "Running.\n");

        while ((st = global_state_wait_for_shutdown_request(&l,
                     REQ_TIMEDOUT)) ==
               LAGOPUS_RESULT_TIMEDOUT) {
          lagopus_msg_debug(5, "waiting shutdown request...\n");
        }
        if (st == LAGOPUS_RESULT_OK) {
          if ((st = global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN)) ==
              LAGOPUS_RESULT_OK) {
            if ((st = lagopus_module_shutdown_all(l)) == LAGOPUS_RESULT_OK) {
              if ((st = lagopus_module_wait_all(s_to)) == LAGOPUS_RESULT_OK) {
                fprintf(stderr, "Finished cleanly.\n");
              } else if (st == LAGOPUS_RESULT_TIMEDOUT) {
                fprintf(stderr, "Trying to stop forcibly...\n");
                if ((st = lagopus_module_stop_all()) == LAGOPUS_RESULT_OK) {
                  if ((st = lagopus_module_wait_all(s_to)) ==
                      LAGOPUS_RESULT_OK) {
                    fprintf(stderr, "Stopped forcibly.\n");
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  lagopus_module_finalize_all();

  if (st != LAGOPUS_RESULT_OK) {
    fprintf(stderr, "\n");
    lagopus_perror(st);
  }

  return (st == LAGOPUS_RESULT_OK) ? 0 : 1;
}
