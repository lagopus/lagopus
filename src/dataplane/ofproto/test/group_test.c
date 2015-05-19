/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include "lagopus/flowdb.h"
#include "lagopus/dataplane.h"

#include "pktbuf.h"
#include "packet.h"
#include "lagopus/group.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_group_select_bucket(void) {
  struct lagopus_packet pkt;

  struct bucket_list bucket_list;
  struct bucket bucket1, bucket2, bucket3;
  struct bucket *bucket;

  TAILQ_INIT(&bucket_list);
  TAILQ_INSERT_TAIL(&bucket_list, &bucket1, entry);
  TAILQ_INSERT_TAIL(&bucket_list, &bucket2, entry);
  TAILQ_INSERT_TAIL(&bucket_list, &bucket3, entry);

  bucket1.ofp.weight = 1;
  bucket2.ofp.weight = 1;
  bucket3.ofp.weight = 1;

  pkt.hash64 = 0;
  bucket = group_select_bucket(&pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket1);
  pkt.hash64 = 1;
  bucket = group_select_bucket(&pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket2);
  pkt.hash64 = 2;
  bucket = group_select_bucket(&pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket3);

  pkt.hash64 = 3;
  bucket = group_select_bucket(&pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket1);
  pkt.hash64 = 4;
  bucket = group_select_bucket(&pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket2);
  pkt.hash64 = 5;
  bucket = group_select_bucket(&pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket3);
}
