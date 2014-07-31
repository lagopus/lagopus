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


#include "unity.h"

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/port.h"
#include "pktbuf.h"
#include "pcap.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_lagopus_pcap_init(void) {
  struct port port;
  lagopus_result_t rv;

  rv = lagopus_pcap_init(&port, "");
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_INVALID_ARGS,
                            "invalid filename error.");
}
