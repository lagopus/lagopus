/* %COPYRIGHT% */

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/queue.h>

#include "unity.h"
#include "lagopus/datastore/bridge.h"
#include "lagopus/dp_apis.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowdb.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/ofcache.h"
#include "pktbuf.h"
#include "packet.h"
#include "mbtree.h"

#include "datapath_test_misc.h"

#include "mbtree.c"

void
setUp(void) {
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
}

void
tearDown(void) {
  dp_api_fini();
}

void
test_get_shifted_value(void) {
  uint8_t oxm_value[8];
  uint32_t oxm_length;
  int shift;
  int keylen;
  uint8_t key[8];
  int i;

  oxm_value[0] = 1;
  oxm_value[1] = 2;
  oxm_value[2] = 3;
  oxm_value[3] = 4;
  oxm_value[4] = 5;
  oxm_value[5] = 6;
  oxm_value[6] = 7;
  oxm_value[7] = 8;
  oxm_length = 8;
  shift = 0;
  keylen = 64;
  get_shifted_value(oxm_value, oxm_length, shift, keylen >> 3, key);
  for (i = 0; i < 8; i++) {
    TEST_ASSERT_EQUAL(key[i], oxm_value[i]);
  }
}
