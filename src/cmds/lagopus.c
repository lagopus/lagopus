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
#include "lagopus_version.h"
#include "agent.h"

#include "lagopus/datastore.h"





static void
s_term_handler(int sig) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  global_state_t gs = GLOBAL_STATE_UNKNOWN;

  if ((r = global_state_get(&gs)) == LAGOPUS_RESULT_OK) {

    if ((int)gs == (int)GLOBAL_STATE_STARTED) {

      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;
      if (sig == SIGTERM || sig == SIGINT) {
        l = SHUTDOWN_GRACEFULLY;
      } else if (sig == SIGQUIT) {
        l = SHUTDOWN_RIGHT_NOW;
      }
      if (IS_VALID_SHUTDOWN(l) == true) {
        lagopus_msg_info("About to request shutdown(%s)...\n",
                         (l == SHUTDOWN_RIGHT_NOW) ?
                         "RIGHT_NOW" : "GRACEFULLY");
        if ((r = global_state_request_shutdown(l)) == LAGOPUS_RESULT_OK) {
          lagopus_msg_info("The shutdown request accepted.\n");
        } else {
          lagopus_perror(r);
          lagopus_msg_error("can't request shutdown.\n");
        }
      }

    } else if ((int)gs < (int)GLOBAL_STATE_STARTED) {
      if (sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        lagopus_abort_before_mainloop();
      }
    } else {
      lagopus_msg_debug(5, "The system is already shutting down.\n");
    }
  }

}


static void
s_hup_handler(int sig) {
  (void)sig;

  if (lagopus_log_get_destination(NULL) == LAGOPUS_LOG_EMIT_TO_FILE) {
    lagopus_msg_info("The log file turned over.\n");
    (void)lagopus_log_reinitialize();
    lagopus_msg_info("The log file turned over.\n");
  }
}





static const char *s_progname;
static const char *s_logfile;
static const char *s_pidfile;
static const char *s_configfile;

static uint16_t s_debug_level = 0;


struct option s_longopts[] = {
  { "debug",   no_argument,        NULL, 'd' },
  { "help",    no_argument,        NULL, 'h' },
  { "version", no_argument,        NULL, 'v' },
  { "logfile", required_argument,  NULL, 'l' },
  { "pidfile", required_argument,  NULL, 'p' },
  { "config",  required_argument,  NULL, 'C' },
};


static void
usage(FILE *fd, int exit_status) {
  if (exit_status != 0) {
    fprintf(stderr, "Try `%s --help' for more information.\n", s_progname);
  } else {
    fprintf(fd, "usage : %s [OPTION...]\n\n\
-d, --debug              Runs in debug mode\n\
-v, --version            Print program version\n\
-h|-?, --help            Display this help and exit\n\
-l, --logfile filename   Specify a log/trace file path (default: syslog)\n\
-p, --pidfile filename   Specify a pid file path (default: /var/run/%s.pid)\n\
-C, --config filename    Speficy a config file path (default: lagopus.dsl)\n\
\n", s_progname, s_progname);
    lagopus_module_usage_all(fd);
  }
  exit(exit_status);
  /* NOTREACHED */
}


static void
parse_args(int argc, const char *const argv[]) {
  int o;

  /*
   * FIXME:
   *	Avoid to use getopt() for proper multi-modules initialization.
   */
  while ((o = getopt_long(argc, (char * const *)argv,
                          "dh?vl:p:C:", s_longopts, NULL)) != EOF) {
    switch (o) {
      case 0: {
        break;
      }
      case 'd': {
        s_debug_level++;
        break;
      }
      case 'h':
      case '?': {
        usage(stdout, 0);
        break;
      }
      case 'v': {
        fprintf(stdout, "%s version %d.%d.%d%s\n",
                LAGOPUS_PRODUCT_NAME,
                LAGOPUS_VERSION_MAJOR,
                LAGOPUS_VERSION_MINOR,
                LAGOPUS_VERSION_PATCH,
                LAGOPUS_VERSION_RELEASE);
        exit(0);
        /* NOTREACHED */
      }
      case 'l': {
        s_logfile = optarg;
        break;
      }
      case 'p': {
        s_pidfile = optarg;
        break;
      }
      case 'C': {
        s_configfile = optarg;
        break;
      }
      default: {
        usage(stderr, 1);
        break;
      }
    }
  }
}





int
main(int argc, const char *const argv[]) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_log_destination_t dst = LAGOPUS_LOG_EMIT_TO_UNKNOWN;
  const char *logarg;
  uint16_t cur_debug_level = 0;

  s_progname = lagopus_get_command_name();
  if (IS_VALID_STRING(s_progname) == false) {
    (void)lagopus_set_command_name(argv[0]);
    s_progname = lagopus_get_command_name();
  }

  parse_args(argc, argv);

  if (IS_VALID_STRING(s_configfile) == true) {
    if ((r = datastore_set_config_file(s_configfile)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(r);
      lagopus_msg_error("can't use '%s' as a configuration file.\n",
                        s_configfile);
      goto bailout;
    }
  }
  s_configfile = NULL;
  (void)datastore_get_config_file(&s_configfile);

  if (IS_VALID_STRING(s_logfile) == true) {
    dst = LAGOPUS_LOG_EMIT_TO_FILE;
    logarg = s_logfile;
  } else {
    logarg = NULL;
    dst = lagopus_log_get_destination(&logarg);
    if (dst == LAGOPUS_LOG_EMIT_TO_UNKNOWN) {
      dst = LAGOPUS_LOG_EMIT_TO_SYSLOG;
      logarg = s_progname;
    }
  }

  if (IS_VALID_STRING(s_pidfile) == true) {
    lagopus_set_pidfile(s_pidfile);
  }

  cur_debug_level = lagopus_log_get_debug_level();

  (void)lagopus_log_initialize(dst, logarg, false, true,
                               (cur_debug_level > s_debug_level) ?
                               cur_debug_level : s_debug_level);

  (void)lagopus_signal(SIGHUP, s_hup_handler, NULL);
  (void)lagopus_signal(SIGINT, s_term_handler, NULL);
  (void)lagopus_signal(SIGTERM, s_term_handler, NULL);
  (void)lagopus_signal(SIGQUIT, s_term_handler, NULL);

  if (s_debug_level == 0) {
    r = lagopus_mainloop_with_callout(argc, argv, NULL, NULL,
                                      true, true, false);
  } else {
    r = lagopus_mainloop_with_callout(argc, argv, NULL, NULL,
                                      false, false, false);
  }

bailout:
  return (r == LAGOPUS_RESULT_OK) ? 0 : 1;
}
