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
#include <rte_config.h>
#include <rte_mbuf.h>
#include "dump_pkts.h"

lagopus_result_t
setup(void *pkts, size_t size) {
  (void) pkts;
  (void) size;
  printf("call setup.\n");
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
teardown(void *pkts, size_t size) {
  (void) pkts;
  (void) size;
  printf("call teardown.\n");
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dump_pkts(void *pkts, size_t size) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i, j;
  struct rte_mbuf **mbufs;

  if (pkts != NULL) {
    mbufs = pkts;

    for (i = 0; i < size; i++) {
      for (j = 0; j < mbufs[i]->data_len; j++) {
        printf("%02x ", *((u_char *) (mbufs[i]->buf_addr + mbufs[i]->data_off) + j ));
      }
      printf("\n\n");
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
