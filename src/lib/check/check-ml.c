#include "lagopus_apis.h"


int
main(int argc, const char * const argv[]) {
  size_t n_workers = 1;

  if (IS_VALID_STRING(argv[1]) == true) {
    size_t tmp = 1;
    lagopus_result_t r = lagopus_str_parse_uint64(argv[1], &tmp);
    if (r == LAGOPUS_RESULT_OK && tmp <= 4) {
      n_workers = tmp;
    }
  }

  (void)lagopus_mainloop_set_callout_workers_number(n_workers);
  (void)lagopus_mainloop_with_callout(argc, argv, NULL, NULL,
                                    false, false, true);
  (void)global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  lagopus_mainloop_wait_thread();

  return 0;
}
