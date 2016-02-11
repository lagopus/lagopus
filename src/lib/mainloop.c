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





#define ONE_SEC	1000LL * 1000LL * 1000LL

#define MAINLOOP_IDLE_PROC_INTERVAL	ONE_SEC
#define MAINLOOP_SHUTDOWN_TIMEOUT	5LL * ONE_SEC

#define DEFAULT_PIDFILE_DIR	"/var/run/"





typedef lagopus_result_t (*mainloop_proc_t)(
    int argc, const char * const argv[],
    lagopus_mainloop_startup_hook_proc_t pre_hook,
    lagopus_mainloop_startup_hook_proc_t post_hook,
    int ipcfd);


typedef struct {
  mainloop_proc_t m_proc;
  int m_argc;
  const char * const * m_argv;
  lagopus_mainloop_startup_hook_proc_t m_pre_hook;
  lagopus_mainloop_startup_hook_proc_t m_post_hook;
  int m_ipcfd;
} mainloop_args_t;





static shutdown_grace_level_t s_gl = SHUTDOWN_UNKNOWN;

static char s_pidfile_buf[PATH_MAX];
static const char *s_pidfile;
static volatile bool s_do_pidfile = false;

static volatile bool s_do_fork = false;
static volatile bool s_do_abort = false;

static size_t s_n_callout_workers = 1;

static lagopus_chrono_t s_idle_proc_interval = 
    MAINLOOP_IDLE_PROC_INTERVAL;
static lagopus_chrono_t s_shutdown_timeout = 
    MAINLOOP_SHUTDOWN_TIMEOUT;


static lagopus_thread_t s_thd = NULL;
static mainloop_args_t s_args = { NULL };





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
    (void)dup2(2, i);
    (void)close(i);
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
             lagopus_get_command_name());
  }

  /* TODO: umask(2) */

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


static lagopus_result_t
s_mainloop_thd_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)tptr;

  if (likely(arg != NULL)) {
    mainloop_args_t *mlarg = (mainloop_args_t *)arg;
    if (likely(mlarg->m_proc != NULL &&
               mlarg->m_argc > 0 &&
               mlarg->m_argv != NULL &&
               IS_VALID_STRING(mlarg->m_argv[0]) == true)) {
      ret = mlarg->m_proc(mlarg->m_argc,
                          mlarg->m_argv,
                          mlarg->m_pre_hook,
                          mlarg->m_post_hook,
                          mlarg->m_ipcfd);
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_start_mainloop_thd(mainloop_proc_t proc, int argc, const char * const argv[],
                     lagopus_mainloop_startup_hook_proc_t pre_hook,
                     lagopus_mainloop_startup_hook_proc_t post_hook,
                     int ipcfd) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(proc != NULL &&
             argc > 0 &&
             argv != NULL &&
             IS_VALID_STRING(argv[0]) == true)) {

    s_args.m_proc = proc;
    s_args.m_argc = argc;
    s_args.m_argv = argv;
    s_args.m_pre_hook = pre_hook;
    s_args.m_post_hook = post_hook;
    s_args.m_ipcfd = ipcfd;

    ret = lagopus_thread_create(&s_thd,
                                s_mainloop_thd_main,
                                NULL,
                                NULL,
                                "mainlooper",
                                (void *)&s_args);
    if (likely(ret == LAGOPUS_RESULT_OK)) {
      ret = lagopus_thread_start(&s_thd, false);
      if (likely(ret == LAGOPUS_RESULT_OK)) {
        global_state_t s;
        shutdown_grace_level_t l;

        ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
        if (likely(ret == LAGOPUS_RESULT_OK)) {
          if (unlikely(s != GLOBAL_STATE_STARTED)) {
            ret = LAGOPUS_RESULT_INVALID_STATE;
          }
        }
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_wait_mainloop_thd(void) {
  lagopus_result_t r = 
      lagopus_thread_wait(&s_thd, MAINLOOP_SHUTDOWN_TIMEOUT);
  if (likely(r == LAGOPUS_RESULT_OK)) {
    lagopus_thread_destroy(&s_thd);
  } else if (r == LAGOPUS_RESULT_TIMEDOUT) {
    lagopus_msg_warning("the mainloop thread doesn't stop. cancel it.\n");
    r = lagopus_thread_cancel(&s_thd);
    if (likely(r == LAGOPUS_RESULT_OK)) {
      r = lagopus_thread_wait(&s_thd, MAINLOOP_SHUTDOWN_TIMEOUT);
      if (likely(r == LAGOPUS_RESULT_OK)) {
        lagopus_thread_destroy(&s_thd);
      } else if (r == LAGOPUS_RESULT_TIMEDOUT) {
        lagopus_msg_error("the mainloop thread still exists.\n");
      }
    } else {
      lagopus_perror(r);
      lagopus_msg_error("can't cancel the mainloop thread.\n");
    }
  } else {
    lagopus_perror(r);
    lagopus_msg_error("failed to wait the mainloop thread stop.\n");
  }
}





static inline lagopus_result_t
s_prologue(int argc, const char * const argv[],
           lagopus_mainloop_startup_hook_proc_t pre_hook,
           lagopus_mainloop_startup_hook_proc_t post_hook,
           int ipcfd) {
  int st;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("%s: Initializing all the modules.\n",
                   lagopus_get_command_name());

  (void)global_state_set(GLOBAL_STATE_INITIALIZING);

  ret = lagopus_module_initialize_all(argc, (const char * const *)argv);
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    lagopus_msg_info("%s: All the modules are initialized.\n",
                     lagopus_get_command_name());                     

    if (pre_hook != NULL) {
      ret = pre_hook(argc, argv);
      if (unlikely(ret != LAGOPUS_RESULT_OK)) {
        lagopus_perror(ret);
        lagopus_msg_error("pre-startup hook failed.\n");
        goto done;
      }
    }

    lagopus_msg_info("%s: Starting all the modules.\n",
                     lagopus_get_command_name());
    (void)global_state_set(GLOBAL_STATE_STARTING);

    ret = lagopus_module_start_all();
    if (likely(ret == LAGOPUS_RESULT_OK)) {

      lagopus_msg_info("%s: All the modules are started and ready to go.\n",
                       lagopus_get_command_name());

      (void)global_state_set(GLOBAL_STATE_STARTED);

      if (post_hook != NULL) {
        ret = post_hook(argc, argv);
        if (unlikely(ret != LAGOPUS_RESULT_OK)) {
          lagopus_perror(ret);
          lagopus_msg_error("post-startup hook failed.\n");
          goto done;
        }
      }
    }
  }

done:
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    if (s_do_pidfile == true) {
      s_gen_pidfile();
    }
    st = 0;
  } else {
    st = 1;
  }
  if (s_do_fork == true && ipcfd >= 0) {
    (void)write(ipcfd, &st, sizeof(int));
    (void)close(ipcfd);
  }

  return ret;
}


static inline lagopus_result_t
s_epilogue(shutdown_grace_level_t l, lagopus_chrono_t to) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);

  lagopus_msg_info("%s: About to shutdown all the modules...\n",
                   lagopus_get_command_name());

  ret = lagopus_module_shutdown_all(l);
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    ret = lagopus_module_wait_all(to);
    if (likely(ret == LAGOPUS_RESULT_OK)) {
      lagopus_msg_info("%s: Shutdown all the modules succeeded.\n",
                       lagopus_get_command_name());
    } else if (ret == LAGOPUS_RESULT_TIMEDOUT) {
   do_cancel:
      lagopus_msg_warning("%s: Shutdown failed. Trying to stop al the module "
                          "forcibly...\n", lagopus_get_command_name());
      ret = lagopus_module_stop_all();
      if (likely(ret == LAGOPUS_RESULT_OK)) {
        ret = lagopus_module_wait_all(to);
        if (likely(ret == LAGOPUS_RESULT_OK)) {
          lagopus_msg_warning("%s: All the modules are stopped forcibly.\n",
                              lagopus_get_command_name());
        } else {
          lagopus_perror(ret);
          lagopus_msg_error("%s: can't stop all the modules.\n",
                            lagopus_get_command_name());
        }
      }
    }
  } else if (ret == LAGOPUS_RESULT_TIMEDOUT) {
    goto do_cancel;
  }

  lagopus_module_finalize_all();

  if (s_do_pidfile == true) {
    s_del_pidfile();
  }

  return ret;
}





static inline lagopus_result_t
s_default_idle_proc(void) {
  return global_state_wait_for_shutdown_request(&s_gl,
                                                s_idle_proc_interval);
}


static inline lagopus_result_t
s_default_mainloop(int argc, const char * const argv[],
                   lagopus_mainloop_startup_hook_proc_t pre_hook,
                   lagopus_mainloop_startup_hook_proc_t post_hook,
                   int ipcfd) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_do_abort == false)) {
    ret = s_prologue(argc, argv, pre_hook, post_hook, ipcfd);
    if (likely(ret == LAGOPUS_RESULT_OK &&
               s_do_abort == false)) {
      lagopus_msg_info("%s: Entering the main loop.\n",
                       lagopus_get_command_name());
      while ((ret = s_default_idle_proc()) == LAGOPUS_RESULT_TIMEDOUT) {
        lagopus_msg_debug(10, "%s: Wainitg for the shutdown request...\n",
                          lagopus_get_command_name());
      }
      if (likely(ret == LAGOPUS_RESULT_OK)) {
        (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
      }

      ret = s_epilogue(s_gl, s_shutdown_timeout);
    }
  }

  return ret;
}





static lagopus_result_t
s_callout_idle_proc(void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  shutdown_grace_level_t *glptr = (shutdown_grace_level_t *)arg;
  shutdown_grace_level_t gl = SHUTDOWN_UNKNOWN;

  lagopus_msg_debug(10, "Checking the shutdown request...\n");

  ret = global_state_wait_for_shutdown_request(&gl, 0LL);
  if (unlikely(ret == LAGOPUS_RESULT_OK)) {
    lagopus_msg_debug(1, "Got the shutdown request.\n");

    if (glptr != NULL) {
      *glptr = gl;
    }
    /*
     * Got a shutdown request. Accept it anyway.
     */
    (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
    /*
     * And return <0 to stop the callout main loop.
     */
    ret = LAGOPUS_RESULT_ANY_FAILURES;
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}


static inline lagopus_result_t
s_callout_mainloop(int argc, const char * const argv[],
                   lagopus_mainloop_startup_hook_proc_t pre_hook,
                   lagopus_mainloop_startup_hook_proc_t post_hook,
                   int ipcfd) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_do_abort == false)) {
    bool handler_inited = false;

    /*
     * Initialize the callout handler.
     */
    ret = lagopus_callout_initialize_handler(s_n_callout_workers,
                                             s_callout_idle_proc,
                                             (void *)&s_gl,
                                             s_idle_proc_interval,
                                             NULL);
    if (likely(ret == LAGOPUS_RESULT_OK)) {
      handler_inited = true;
      if (s_do_abort == false) {
        ret = s_prologue(argc, argv, pre_hook, post_hook, ipcfd);
        if (likely(ret == LAGOPUS_RESULT_OK &&
                   s_do_abort == false)) {
          lagopus_result_t r1 = LAGOPUS_RESULT_ANY_FAILURES;
          lagopus_result_t r2 = LAGOPUS_RESULT_ANY_FAILURES;

          lagopus_msg_info("%s is a go.\n", lagopus_get_command_name());

          /*
           * Start the callout handler main loop.
           */
          ret = lagopus_callout_start_main_loop();

          /*
           * No matter the ret is, here we have the main loop stopped. To
           * make it sure that the loop stopping call the stop API in case.
           */
          r1 = lagopus_callout_stop_main_loop();

          r2 = s_epilogue(s_gl, s_shutdown_timeout);

          if (ret == LAGOPUS_RESULT_OK) {
            ret = r1;
          }
          if (ret == LAGOPUS_RESULT_OK) {
            ret = r2;
          }
        }
      }
      
      if (handler_inited == true) {
        lagopus_callout_finalize_handler();
      }
    }
  }

  return ret;
}





static inline void
s_set_failsafe_handler(int sig) {
  sighandler_t h = NULL;

  if (lagopus_signal(sig, SIG_CUR, &h) == LAGOPUS_RESULT_OK) {
    if (h == NULL || h == SIG_DFL || h == SIG_IGN) {
      lagopus_result_t r = lagopus_signal(SIGQUIT, s_term_handler, NULL);
      if (r == LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("The signal %d seems not to be properly handled. "
                            "Set a decent handler.\n", sig);
      } else {
        lagopus_perror(r);
          lagopus_msg_warning("Can't set failsefa signal handler to "
                              "signal %d.\n", sig);
      }
    }
  }
}
  

static inline lagopus_result_t
s_do_mainloop(mainloop_proc_t mainloopproc,
              int argc, const char * const argv[],
              lagopus_mainloop_startup_hook_proc_t pre_hook,
              lagopus_mainloop_startup_hook_proc_t post_hook,
              bool do_fork, bool do_pidfile, bool do_thread) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(mainloopproc != NULL)) {

    s_do_pidfile = do_pidfile;
    s_do_fork = do_fork;

    /*
     * failsafe.
     */
    if (do_fork == false) {
      s_set_failsafe_handler(SIGINT);
    }
    s_set_failsafe_handler(SIGTERM);
    s_set_failsafe_handler(SIGQUIT);

    if (do_fork == true) {
      int ipcfds[2];
      int st;

      errno = 0;
      st = pipe(ipcfds);
      if (likely(st == 0)) {
        pid_t pid;

        errno = 0;
        pid = fork();
        if (pid > 0) {
          /*
           * Parent.
           */
          int child_st = -1;
          ssize_t ssz;

          (void)close(ipcfds[1]);

          errno = 0;
          ssz = read(ipcfds[0], &child_st, sizeof(int));
          if (likely(ssz == sizeof(int))) {
            ret = (child_st == 0) ?
                LAGOPUS_RESULT_OK : LAGOPUS_RESULT_ANY_FAILURES;
          } else {
            ret = LAGOPUS_RESULT_POSIX_API_ERROR;
          }
          (void)close(ipcfds[0]);
        } else if (pid == 0) {
          /*
           * Child.
           */
          (void)close(ipcfds[0]);
          s_daemonize(ipcfds[1]);
          if (lagopus_log_get_destination(NULL) == LAGOPUS_LOG_EMIT_TO_FILE) {
            (void)lagopus_log_reinitialize();
          }
          if (do_thread == false) {
            ret = mainloopproc(argc, argv, pre_hook, post_hook, ipcfds[1]);
          } else {
            ret = s_start_mainloop_thd(mainloopproc, argc, argv,
                                       pre_hook, post_hook, ipcfds[1]);
          }
        } else {
          ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        }          

      } else {
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }

    } else {	/* do_fork == true */

      if (do_thread == false) {
        ret = mainloopproc(argc, argv, pre_hook, post_hook, -1);
      } else {
        ret = s_start_mainloop_thd(mainloopproc, argc, argv,
                                   pre_hook, post_hook, -1);
      }

    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





void
lagopus_abort_before_mainloop(void) {
  s_do_abort = true;
  mbar();
}


bool
lagopus_is_abort_before_mainloop(void) {
  return s_do_abort;
}


lagopus_result_t
lagopus_set_pidfile(const char *file) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(file) == true) {
    const char *newfile = strdup(file);
    if (IS_VALID_STRING(newfile) == true) {
      if (IS_VALID_STRING(s_pidfile) == true) {
        free((void *)s_pidfile);
      }
      s_pidfile = newfile;
      ret = LAGOPUS_RESULT_OK;
    } else {
      /*
       * Note: scan-build warns a potentialy memory leak here and yes
       * it is intended to be.
       */
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_create_pidfile(void) {
  s_gen_pidfile();
}


void
lagopus_remove_pidfile(void) {
  s_del_pidfile();
}


lagopus_result_t
lagopus_mainloop_set_callout_workers_number(size_t n) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (n > 4) {
    ret = LAGOPUS_RESULT_TOO_LARGE;
  } else {
    s_n_callout_workers = n;
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}


lagopus_result_t
lagopus_mainloop_set_shutdown_check_interval(lagopus_chrono_t nsec) {
  if (nsec < ONE_SEC) {
    return LAGOPUS_RESULT_TOO_SMALL;
  } else if (nsec >= (10LL * ONE_SEC)) {
    return LAGOPUS_RESULT_TOO_LARGE;
  }

  s_idle_proc_interval = nsec;
  return LAGOPUS_RESULT_OK;
}


lagopus_result_t
lagopus_mainloop_set_shutdown_timeout(lagopus_chrono_t nsec) {
  if (nsec < ONE_SEC) {
    return LAGOPUS_RESULT_TOO_SMALL;
  } else if (nsec >= (30LL * ONE_SEC)) {
    return LAGOPUS_RESULT_TOO_LARGE;
  }

  s_shutdown_timeout = nsec;
  return LAGOPUS_RESULT_OK;
}
  




lagopus_result_t
lagopus_mainloop(int argc, const char * const argv[],
                 lagopus_mainloop_startup_hook_proc_t pre_hook,
                 lagopus_mainloop_startup_hook_proc_t post_hook,
                 bool do_fork, bool do_pidfile, bool do_thread) {
  return s_do_mainloop(s_default_mainloop, argc, argv,
                       pre_hook, post_hook,
                       do_fork, do_pidfile, do_thread);
}


lagopus_result_t
lagopus_mainloop_with_callout(int argc, const char * const argv[],
                              lagopus_mainloop_startup_hook_proc_t pre_hook,
                              lagopus_mainloop_startup_hook_proc_t post_hook,
                              bool do_fork, bool do_pidfile,
                              bool do_thread) {
  return s_do_mainloop(s_callout_mainloop, argc, argv,
                       pre_hook, post_hook,
                       do_fork, do_pidfile, do_thread);
}


void
lagopus_mainloop_wait_thread(void) {
  if (s_thd != NULL) {
    s_wait_mainloop_thd();
  }
}


lagopus_result_t
lagopus_mainloop_prologue(int argc, const char * const argv[],
                          lagopus_mainloop_startup_hook_proc_t pre_hook,
                          lagopus_mainloop_startup_hook_proc_t post_hook) {
  return s_prologue(argc, argv, pre_hook, post_hook, -1);
}


lagopus_result_t
lagopus_mainloop_epilogue(shutdown_grace_level_t l, lagopus_chrono_t to) {
  return s_epilogue(l, to);
}
