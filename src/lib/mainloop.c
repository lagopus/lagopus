/* %COPYRIGHT% */


#include "lagopus_apis.h"





#define ONE_SEC	1000LL * 1000LL * 1000LL





static const char *s_progname = "\"The application name not set\"";
static shutdown_grace_level_t s_gl = SHUTDOWN_UNKNOWN;





static inline lagopus_result_t
s_prologue(int argc, const char * const argv[],
           lagopus_mainloop_module_startup_hook_proc_t hook) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *appname = (char *)lagopus_get_command_name();

  if (IS_VALID_STRING(appname) == true) {
    s_progname = appname;
  }

  lagopus_msg_info("%s: Initializing all the modules.\n", s_progname);

  (void)global_state_set(GLOBAL_STATE_INITIALIZING);

  ret = lagopus_module_initialize_all(argc, (const char * const *)argv);
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    lagopus_msg_info("%s: All the modules are initialized.\n", s_progname);

    if (hook != NULL) {
      ret = (hook)();
    }
    if (unlikely(ret != LAGOPUS_RESULT_OK)) {
      lagopus_perror(ret);
      lagopus_msg_error("%s: Module startup hook failures.\n", s_progname);
      return ret;
    }

    lagopus_msg_info("%s: Starting all the modules.\n", s_progname);

    (void)global_state_set(GLOBAL_STATE_STARTING);

    ret = lagopus_module_start_all();
    if (likely(ret == LAGOPUS_RESULT_OK)) {

      lagopus_msg_info("%s: All the modules are started and ready to go.\n",
                       s_progname);

      (void)global_state_set(GLOBAL_STATE_STARTED);
    }
  }

  return ret;
}


static inline lagopus_result_t
s_epilogue(shutdown_grace_level_t l, lagopus_chrono_t to) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);

  lagopus_msg_info("%s: About to shutdown all the modules...\n", s_progname);

  ret = lagopus_module_shutdown_all(l);
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    ret = lagopus_module_wait_all(to);
    if (likely(ret == LAGOPUS_RESULT_OK)) {
      lagopus_msg_info("%s: Shutdown all the modules succeeded.\n",
                       s_progname);
    } else if (ret == LAGOPUS_RESULT_TIMEDOUT) {
   do_cancel:
      lagopus_msg_warning("%s: Shutdown failed. Trying to stop al the module "
                          "forcibly...\n", s_progname);
      ret = lagopus_module_stop_all();
      if (likely(ret == LAGOPUS_RESULT_OK)) {
        ret = lagopus_module_wait_all(to);
        if (likely(ret == LAGOPUS_RESULT_OK)) {
          lagopus_msg_warning("%s: All the modules are stopped forcibly.\n",
                              s_progname);
        } else {
          lagopus_perror(ret);
          lagopus_msg_error("%s: can't stop all the modules.\n", s_progname);
        }
      }
    }
  } else if (ret == LAGOPUS_RESULT_TIMEDOUT) {
    goto do_cancel;
  }

  lagopus_module_finalize_all();

  return ret;
}


static inline lagopus_result_t
s_default_idle_proc(void) {
  return global_state_wait_for_shutdown_request(&s_gl, ONE_SEC);
}


static inline lagopus_result_t
s_default_mainloop(int argc, const char * const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = s_prologue(argc, argv, NULL);
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    lagopus_msg_info("%s: Entering the main loop.\n", s_progname);

    while ((ret = s_default_idle_proc()) == LAGOPUS_RESULT_TIMEDOUT) {
      lagopus_msg_debug(10, "%s: Wainitg for the shutdown request...\n",
                        s_progname);
    }
    if (likely(ret == LAGOPUS_RESULT_OK)) {
      (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
    }

    ret = s_epilogue(s_gl, ONE_SEC * 5LL);
  }

  return ret;
}





lagopus_result_t
lagopus_mainloop(int argc, const char * const argv[],
                 lagopus_mainloop_proc_t proc) {
  return (proc != NULL) ?
      proc(argc, argv) :
      s_default_mainloop(argc, argv);
}


lagopus_result_t
lagopus_mainloop_prologue(int argc, const char * const argv[], 
                          lagopus_mainloop_module_startup_hook_proc_t hook) {
  return s_prologue(argc, argv, hook);
}


lagopus_result_t
lagopus_mainloop_epilogue(shutdown_grace_level_t l, lagopus_chrono_t to) {
  return s_epilogue(l, to);
}
