/* %COPYRIGHT% */

#include <inttypes.h>
#include <time.h>
#include <sys/queue.h>

#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/flowdb.h"

void
setUp(void) {
  init_dp_timer();
}

void
tearDown(void) {
}

void
test_add_flow_timer(void) {
  struct flow flow;
  lagopus_result_t rv;

  flow.idle_timeout = 100;
  flow.hard_timeout = 100;
  rv = add_flow_timer(&flow);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}
