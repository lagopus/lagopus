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

#include "agent.h"
#include "confsys.h"





#define ONE_SEC		1000LL * 1000LL * 1000LL
#define REQ_TIMEDOUT	ONE_SEC

#define DEFAULT_PIDFILE_DIR	"/var/run/"





static lagopus_chrono_t s_to = 1000LL * 1000LL * 1000LL * 5;





static inline pid_t
s_setsid(void) {
  pid_t ret = (pid_t)-1;
  int fd = open("/dev/tty", O_RDONLY);

  if (fd >= 0) {
    (void)close(fd);
    if ((ret = setsid()) < 0) {
#ifdef TIOCNOTTY
      int one = 1;
      fd = open("/dev/tty", O_RDONLY);
      if (fd >= 0) {
        (void)ioctl(fd, TIOCNOTTY, &one);
      }
#endif /* TIOCNOTTY */
      (void)close(fd);
      ret = getpid();
      (void)setpgid(0, ret);
    }
  } else {
    if ((ret = setsid()) < 0) {
      ret = getpid();
      (void)setpgid(0, ret);
    }
  }

  return ret;
}


static inline void
s_daemonize(int exclude_fd) {
  int i;

  (void)s_setsid();

  for (i = 0; i < 1024; i++) {
    if (i != exclude_fd) {
      (void)close(i);
    }
  }

  i = open("/dev/zero", O_RDONLY);
  if (i > 0) {
    (void)dup2(0, i);
    (void)close(i);
  }

  i = open("/dev/null", O_WRONLY);
  if (i > 1) {
    (void)dup2(1, i);
    (void)close(i);
  }

  i = open("/dev/null", O_WRONLY);
  if (i > 2) {
    (void)dup2(1, i);
    (void)close(i);
  }
}





static volatile bool s_got_term_sig = false;


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
        s_got_term_sig = true;
      }
    } else {
      lagopus_msg_debug(5, "The system is already shutting down.\n");
    }
  }

}


static void
s_hup_handler(int sig) {
  (void)sig;

  if (lagopus_log_get_destination() == LAGOPUS_LOG_EMIT_TO_FILE) {
    lagopus_msg_info("The log file turned over.\n");
    (void)lagopus_log_reinitialize();
    lagopus_msg_info("The log file turned over.\n");
  }
}





static const char *s_progname;
static const char *s_logfile;
static const char *s_pidfile;
static const char *s_conffile;
static char s_pidfile_buf[PATH_MAX];

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
-C, --config filename    Speficy a config file path (default: lagopus.conf)\n\
\n", s_progname, s_progname);
    lagopus_module_usage_all(fd);
  }
  exit(exit_status);
}


static void
parse_args(int argc, const char *const argv[]) {
  int o;

  /*
   * FIXME:
   *	Avoid to use getopt() for proper multi-modules initialization.
   */
  while ((o = getopt_long(argc, (char *const *)argv,
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
        fprintf(stdout, "version 0.1\n");
        exit(0);
        break;
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
        s_conffile = optarg;
        break;
      }
      default: {
        usage(stderr, 1);
        break;
      }
    }
  }
}


static inline void
s_gen_pidfile(void) {
  FILE *fd = NULL;

  if (IS_VALID_STRING(s_pidfile) == true) {
    snprintf(s_pidfile_buf, sizeof(s_pidfile_buf), "%s", s_pidfile);
  } else {
    snprintf(s_pidfile_buf, sizeof(s_pidfile_buf),
             DEFAULT_PIDFILE_DIR "%s.pid",
             s_progname);
  }

  fd = fopen(s_pidfile_buf, "w");
  if (fd != NULL) {
    fprintf(fd, "%d\n", (int)getpid());
    fflush(fd);
    (void)fclose(fd);
  } else {
    lagopus_perror(LAGOPUS_RESULT_POSIX_API_ERROR);
    lagopus_msg_error("can't create a pidfile \"%s\".\n",
                      s_pidfile_buf);
  }
}


static inline void
s_del_pidfile(void) {
  if (IS_VALID_STRING(s_pidfile_buf) == true) {
    struct stat st;

    if (stat(s_pidfile_buf, &st) == 0) {
      if (S_ISREG(st.st_mode)) {
        (void)unlink(s_pidfile_buf);
      }
    }
  }
}


static inline int
s_do_main(int argc, const char *const argv[], int ipcfd) {
  lagopus_result_t st = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("Initializing all the modules.\n");

  (void)global_state_set(GLOBAL_STATE_INITIALIZING);

  if ((st = lagopus_module_initialize_all(argc,
                                          (const char *const *)argv)) ==
      LAGOPUS_RESULT_OK &&
      s_got_term_sig == false) {

    lagopus_msg_info("All the modules are initialized.\n");

    lagopus_msg_info("Starting all the modules.\n");

    (void)global_state_set(GLOBAL_STATE_STARTING);
    if ((st = lagopus_module_start_all()) ==
        LAGOPUS_RESULT_OK &&
        s_got_term_sig == false) {
      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;

      lagopus_msg_info("All the modules are started and ready to go.\n");

      config_propagate_lagopus_conf(s_conffile);

      (void)global_state_set(GLOBAL_STATE_STARTED);

      if (ipcfd >= 0) {
        int zero = 0;
        (void)write(ipcfd, (void *)&zero, sizeof(int));
        (void)close(ipcfd);
        ipcfd = -1;
      }

      lagopus_msg_info("The Lagopus is a go.\n");

      s_gen_pidfile();

      while ((st = global_state_wait_for_shutdown_request(&l,
                   REQ_TIMEDOUT)) ==
             LAGOPUS_RESULT_TIMEDOUT) {
        lagopus_msg_debug(5, "Waiting for the shutdown request...\n");
      }
      if (st == LAGOPUS_RESULT_OK) {
        (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
        if ((st = lagopus_module_shutdown_all(l)) == LAGOPUS_RESULT_OK) {
          if ((st = lagopus_module_wait_all(s_to)) == LAGOPUS_RESULT_OK) {
            lagopus_msg_info("Shutdown succeeded.\n");
          } else if (st == LAGOPUS_RESULT_TIMEDOUT) {
          do_cancel:
            lagopus_msg_warning("Trying to stop forcibly...\n");
            if ((st = lagopus_module_stop_all()) == LAGOPUS_RESULT_OK) {
              if ((st = lagopus_module_wait_all(s_to)) ==
                  LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("Stopped forcibly.\n");
              }
            }
          }
        } else if (st == LAGOPUS_RESULT_TIMEDOUT) {
          goto do_cancel;
        }
      }
    }
  }

  lagopus_module_finalize_all();

  if (st != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("Bailed out, anyway. The latest result status is:"
                        "%s\n", lagopus_error_get_string(st));
    if (ipcfd >= 0) {
      int one = 1;
      (void)write(ipcfd, (void *)&one, sizeof(int));
      (void)close(ipcfd);
    }
  }

  s_del_pidfile();

  return (st == LAGOPUS_RESULT_OK) ? 0 : 1;
}





int
main(int argc, const char *const argv[]) {
  lagopus_log_destination_t dst = LAGOPUS_LOG_EMIT_TO_UNKNOWN;
  const char *logarg;
  char *p;
  uint16_t cur_debug_level = lagopus_log_get_debug_level();
  int ipcfds[2];

  ipcfds[0] = -1;
  ipcfds[1] = -1;

  s_progname = ((p = strrchr(argv[0], '/')) != NULL) ? ++p : argv[0];

  parse_args(argc, argv);

  if (IS_VALID_STRING(s_logfile) == true) {
    dst = LAGOPUS_LOG_EMIT_TO_FILE;
    logarg = s_logfile;
  } else {
    dst = LAGOPUS_LOG_EMIT_TO_SYSLOG;
    logarg = s_progname;
  }
  /*
   * If the cur_debug_level > 0 the debug level is set by the lagopus
   * runtime so use it as is.
   */
  (void)lagopus_log_initialize(dst, logarg, false, true,
                               (cur_debug_level > 0) ?
                               cur_debug_level : s_debug_level);

  (void)lagopus_signal(SIGHUP, s_hup_handler, NULL);
  (void)lagopus_signal(SIGINT, s_term_handler, NULL);
  (void)lagopus_signal(SIGTERM, s_term_handler, NULL);
  (void)lagopus_signal(SIGQUIT, s_term_handler, NULL);

  if (s_debug_level == 0) {
    pid_t pid = (pid_t)-1;

    if (pipe(ipcfds) != 0) {
      lagopus_perror(LAGOPUS_RESULT_POSIX_API_ERROR);
      return 1;
    }

    pid = fork();
    if (pid > 0) {
      int exit_code;
      ssize_t st;

      (void)close(ipcfds[1]);
      if ((st = read(ipcfds[0], (void *)&exit_code, sizeof(int))) !=
          sizeof(int)) {
        lagopus_perror(LAGOPUS_RESULT_POSIX_API_ERROR);
        return 1;
      }

      return exit_code;

    } else if (pid == 0) {
      s_daemonize(ipcfds[1]);
      if (lagopus_log_get_destination() == LAGOPUS_LOG_EMIT_TO_FILE) {
        (void)lagopus_log_reinitialize();
      }
    } else {
      lagopus_perror(LAGOPUS_RESULT_POSIX_API_ERROR);
      lagopus_msg_error("can' be a daemon.\n");
      return 1;
    }
  }

  return s_do_main(argc, argv, ipcfds[1]);
}
