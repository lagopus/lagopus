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
  struct lagopus_packet *pkt;
  struct port port;

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

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  pkt->in_port = &port;

  pkt->hash64 = 3;
  bucket = group_select_bucket(pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket1);
  pkt->hash64 = 4;
  bucket = group_select_bucket(pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket2);
  pkt->hash64 = 5;
  bucket = group_select_bucket(pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket3);

  pkt->hash64 = 6;
  bucket = group_select_bucket(pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket1);
  pkt->hash64 = 7;
  bucket = group_select_bucket(pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket2);
  pkt->hash64 = 8;
  bucket = group_select_bucket(pkt, &bucket_list);
  TEST_ASSERT_EQUAL(bucket, &bucket3);
}
