/* %COPYRIGHT% */

#include "lagopus_apis.h"





static shutdown_grace_level_t s_gl = SHUTDOWN_GRACEFULLY;
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





static lagopus_result_t
s_idle_proc(void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  shutdown_grace_level_t *glptr = (shutdown_grace_level_t *)arg;
  shutdown_grace_level_t gl = SHUTDOWN_UNKNOWN;

  ret = global_state_wait_for_shutdown_request(&gl, 0LL);
  if (unlikely(ret == LAGOPUS_RESULT_OK)) {
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
s_mainloop(int argc, const char * const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(s_got_term_sig == false)) {
    bool handler_inited = false;

    /*
     * Initialize the callout handler.
     */
    ret = lagopus_callout_initialize_handler(1,
                                             s_idle_proc, (void *)&s_gl,
                                             1000LL * 1000LL * 1000LL,
                                             NULL);
    if (likely(ret == LAGOPUS_RESULT_OK)) {
      handler_inited = true;
      if (s_got_term_sig == false) {
        ret = lagopus_mainloop_prologue(argc, argv, NULL);
        if (likely(ret == LAGOPUS_RESULT_OK &&
                   s_got_term_sig == false)) {
          lagopus_result_t r1 = LAGOPUS_RESULT_ANY_FAILURES;
          lagopus_result_t r2 = LAGOPUS_RESULT_ANY_FAILURES;

          /*
           * Start the callout handler main loop.
           */
          ret = lagopus_callout_start_main_loop();

          /*
           * No matter the ret is, here we have the main loop stopped. To
           * make it sure that the loop stopping call the stop API in case.
           */
          r1 = lagopus_callout_stop_main_loop();
          if (r1 == LAGOPUS_RESULT_ALREADY_HALTED) {
            r1 = LAGOPUS_RESULT_OK;
          }

          r2 = lagopus_mainloop_epilogue(s_gl,
                                         5 * 1000LL * 1000LL * 1000LL);

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





int
main(int argc, const char * const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)lagopus_set_command_name(argv[0]);

  (void)lagopus_signal(SIGINT, s_term_handler, NULL);
  (void)lagopus_signal(SIGTERM, s_term_handler, NULL);
  (void)lagopus_signal(SIGQUIT, s_term_handler, NULL);

  ret = lagopus_mainloop(argc, argv, s_mainloop);

  return (ret == LAGOPUS_RESULT_OK) ? 0 : 1;
}
