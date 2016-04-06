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

#include "unity.h"
#include "lagopus_apis.h"
#include "../ip_addr.c"

void
setUp(void) {
}

void
tearDown(void) {
}

#define IS_IPV4(a)  (((a) == true) ? AF_INET : AF_INET6)

void
test_ip_address_create_and_destroy(void) {
  lagopus_result_t rc;

  const char *name1 = "127.0.0.1";
  const char *name2 = "::1";
  const char *name3 = "localhost";
  //const char *name4 = "QkwUfh2GGN9sjCx";
  const char *name5 = "127.0";
  const char *name6 = "127.0.0.0";
  const char *name7 = "127.0.0.1.1";
  lagopus_ip_address_t *ip1 = NULL;
  lagopus_ip_address_t *ip2 = NULL;
  lagopus_ip_address_t *ip3 = NULL;
  //lagopus_ip_address_t *ip4 = NULL;
  lagopus_ip_address_t *ip5 = NULL;
  lagopus_ip_address_t *ip6 = NULL;
  bool is_ipv4 = false;

  {
    // Normal case(name = IPv4, ai_family = AF_INET)
    rc = lagopus_ip_address_create(name1, true, &ip1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET, (ip1->saddr).ss_family);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      ip1, "lagopus_ip_address_create() will create new ip_address");
    TEST_ASSERT_EQUAL_STRING(name1, ip1->addr_str);
    TEST_ASSERT_EQUAL(AF_INET, IS_IPV4(ip1->is_ipv4));

    rc = lagopus_ip_address_is_ipv4(ip1, &is_ipv4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(true, is_ipv4);
    lagopus_ip_address_destroy(ip1);

    // Normal case(name = IPv6, ai_family = AF_INET6)
    rc = lagopus_ip_address_create(name2, false, &ip2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET6, (ip2->saddr).ss_family);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      ip2, "lagopus_ip_address_create() will create new ip_address");
    TEST_ASSERT_EQUAL_STRING(name2, ip2->addr_str);
    TEST_ASSERT_EQUAL(AF_INET6, IS_IPV4(ip2->is_ipv4));

    rc = lagopus_ip_address_is_ipv4(ip2, &is_ipv4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(false, is_ipv4);
    lagopus_ip_address_destroy(ip2);

    // Normal case(name = IPv4, ai_family = AF_INET6)
    rc = lagopus_ip_address_create(name1, false, &ip1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET, (ip1->saddr).ss_family);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      ip1, "lagopus_ip_address_create() will create new ip_address");
    TEST_ASSERT_EQUAL_STRING(name1, ip1->addr_str);
    TEST_ASSERT_EQUAL(AF_INET, IS_IPV4(ip1->is_ipv4));

    // Normal case(name = IPv6, ai_family = AF_INET)
    rc = lagopus_ip_address_create(name2, true, &ip2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET6, (ip2->saddr).ss_family);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      ip2, "lagopus_ip_address_create() will create new ip_address");
    TEST_ASSERT_EQUAL_STRING(name2, ip2->addr_str);
    TEST_ASSERT_EQUAL(AF_INET6, IS_IPV4(ip2->is_ipv4));

    // Normal case(IP address obtained from a host name,
    //             result addr_str = '127.0.0.1')
    rc = lagopus_ip_address_create(name3, true, &ip3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET, (ip3->saddr).ss_family);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      ip3, "lagopus_ip_address_create() will create new ip_address");
    TEST_ASSERT_EQUAL_STRING(name1, ip3->addr_str);
    TEST_ASSERT_EQUAL(AF_INET, IS_IPV4(ip3->is_ipv4));

    // Normal case(IP address obtained from a host name)
    //rc = lagopus_ip_address_create(name4, true, &ip4);
    //TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);

    // Normal case(name = IPv4, ai_family = AF_INET,
    //             name = '127.0', result addr_str = '127.0.0.0')
    rc = lagopus_ip_address_create(name5, true, &ip5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET, (ip5->saddr).ss_family);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      ip1, "lagopus_ip_address_create() will create new ip_address");
    TEST_ASSERT_EQUAL_STRING(name6, ip5->addr_str);
    TEST_ASSERT_EQUAL(AF_INET, IS_IPV4(ip5->is_ipv4));
    lagopus_ip_address_destroy(ip5);

    // Normal case(IP address bad format.)
    rc = lagopus_ip_address_create(name7, true, &ip6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);
  }

  // Abnormal case
  {
    rc = lagopus_ip_address_create(NULL, true, &ip1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = lagopus_ip_address_create(name1, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(ip1);
  lagopus_ip_address_destroy(ip2);
  lagopus_ip_address_destroy(ip3);
  //lagopus_ip_address_destroy(ip4);
}

void
test_lagopus_ip_address_copy(void) {
  lagopus_result_t rc;

  const char *name1 = "127.0.0.1";
  const char *name2 = "::1";
  lagopus_ip_address_t *actual_ip1 = NULL;
  lagopus_ip_address_t *actual_ip2 = NULL;
  lagopus_ip_address_t *expected_ip1 = NULL;
  lagopus_ip_address_t *expected_ip2 = NULL;

  rc = lagopus_ip_address_create(name1, true, &actual_ip1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_EQUAL_STRING(name1, actual_ip1->addr_str);

  rc = lagopus_ip_address_create(name2, false, &actual_ip2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_EQUAL_STRING(name2, actual_ip2->addr_str);

  // Normal case
  {
    rc = lagopus_ip_address_copy(actual_ip1, &expected_ip1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL((expected_ip1->saddr).ss_family,
                      (actual_ip1->saddr).ss_family);
    TEST_ASSERT_EQUAL_STRING(expected_ip1->addr_str, actual_ip1->addr_str);
    TEST_ASSERT_TRUE(actual_ip1->is_ipv4 == expected_ip1->is_ipv4);

    rc = lagopus_ip_address_copy(actual_ip2, &expected_ip2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL((expected_ip2->saddr).ss_family,
                      (actual_ip2->saddr).ss_family);
    TEST_ASSERT_EQUAL_STRING(expected_ip2->addr_str, actual_ip2->addr_str);
    TEST_ASSERT_TRUE(actual_ip2->is_ipv4 == expected_ip2->is_ipv4);

    rc = lagopus_ip_address_copy(actual_ip2, &actual_ip1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL((expected_ip2->saddr).ss_family,
                      (actual_ip1->saddr).ss_family);
    TEST_ASSERT_EQUAL_STRING(expected_ip2->addr_str, actual_ip1->addr_str);
    TEST_ASSERT_TRUE(actual_ip1->is_ipv4 == expected_ip2->is_ipv4);
  }

  // Abnormal case
  {
    rc = lagopus_ip_address_copy(NULL, &expected_ip1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = lagopus_ip_address_copy(actual_ip1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_ip1);
  lagopus_ip_address_destroy(actual_ip2);
  lagopus_ip_address_destroy(expected_ip1);
  lagopus_ip_address_destroy(expected_ip2);
}

void
test_lagopus_ip_address_equals(void) {
  lagopus_result_t rc;
  bool result;

  const char *name1 = "127.0.0.1";
  const char *name2 = "127.0.0.1";
  const char *name3 = "10.0.0.1";
  const char *name4 = "::1";
  const char *name5 = "0:0:0:0:0:0:0:1";
  const char *name6 = "::0:0:1";
  const char *name7 = "1::1";
  lagopus_ip_address_t *ip1;
  lagopus_ip_address_t *ip2;
  lagopus_ip_address_t *ip3;
  lagopus_ip_address_t *ip4;
  lagopus_ip_address_t *ip5;
  lagopus_ip_address_t *ip6;
  lagopus_ip_address_t *ip7;

  rc = lagopus_ip_address_create(name1, true, &ip1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name2, true, &ip2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name3, true, &ip3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name4, true, &ip4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name5, true, &ip5);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name6, true, &ip6);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name7, true, &ip7);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    // equal
    result = lagopus_ip_address_equals(ip1, ip2);
    TEST_ASSERT_TRUE(result);

    // not equal
    result = lagopus_ip_address_equals(ip1, ip3);
    TEST_ASSERT_FALSE(result);

    // not equal
    result = lagopus_ip_address_equals(ip2, ip3);
    TEST_ASSERT_FALSE(result);

    // equal
    result = lagopus_ip_address_equals(ip4, ip5);
    TEST_ASSERT_TRUE(result);

    // equal
    result = lagopus_ip_address_equals(ip4, ip6);
    TEST_ASSERT_TRUE(result);

    // equal
    result = lagopus_ip_address_equals(ip5, ip6);
    TEST_ASSERT_TRUE(result);

    // not equal
    result = lagopus_ip_address_equals(ip4, ip7);
    TEST_ASSERT_FALSE(result);

    // not equal
    result = lagopus_ip_address_equals(ip1, ip4);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    // not equal
    result = lagopus_ip_address_equals(NULL, NULL);
    TEST_ASSERT_FALSE(result);

    // not equal
    result = lagopus_ip_address_equals(ip1, NULL);
    TEST_ASSERT_FALSE(result);

    // not equal
    result = lagopus_ip_address_equals(NULL, ip1);
    TEST_ASSERT_FALSE(result);
  }

  lagopus_ip_address_destroy(ip1);
  lagopus_ip_address_destroy(ip2);
  lagopus_ip_address_destroy(ip3);
  lagopus_ip_address_destroy(ip4);
  lagopus_ip_address_destroy(ip5);
  lagopus_ip_address_destroy(ip6);
  lagopus_ip_address_destroy(ip7);
}

void
test_ip_address_public_ip_address_str(void) {
  lagopus_result_t rc;
  lagopus_ip_address_t *ip1 = NULL;
  lagopus_ip_address_t *ip2 = NULL;
  const char *name1 = "127.0.0.1";
  const char *name2 = "::1";
  char *actual_addr_str1 = NULL;
  char *actual_addr_str2 = NULL;
  const char *expected_addr_str1 = name1;
  const char *expected_addr_str2 = name2;

  rc = lagopus_ip_address_create(name1, true, &ip1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name2, false, &ip2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = lagopus_ip_address_str_get(ip1, &actual_addr_str1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_addr_str1, actual_addr_str1);

    rc = lagopus_ip_address_str_get(ip2, &actual_addr_str2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_addr_str2, actual_addr_str2);
  }

  // Abnormal case of getter
  {
    rc = lagopus_ip_address_str_get(NULL, &actual_addr_str1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = lagopus_ip_address_str_get(ip1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free(actual_addr_str1);
  free(actual_addr_str2);

  lagopus_ip_address_destroy(ip1);
  lagopus_ip_address_destroy(ip2);
}

void
test_ip_address_public_sockaddr(void) {
  lagopus_result_t rc;
  lagopus_ip_address_t *ip1 = NULL;
  lagopus_ip_address_t *ip2 = NULL;
  lagopus_ip_address_t *ip3 = NULL;
  lagopus_ip_address_t *ip4 = NULL;
  const char *name1 = "127.0.0.1";
  const char *name2 = "::1";
  const char *name3 = "localhost";
  struct sockaddr *actual_sockaddr1 = NULL;
  struct sockaddr *actual_sockaddr2 = NULL;
  struct sockaddr *actual_sockaddr3 = NULL;
  struct sockaddr *actual_sockaddr4 = NULL;
  socklen_t actual_sockaddr_len = 0;

  char ipv4_addr[INET_ADDRSTRLEN];
  char ipv6_addr[INET6_ADDRSTRLEN];

  rc = lagopus_ip_address_create(name1, true, &ip1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name2, false, &ip2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = lagopus_ip_address_create(name3, true, &ip3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // create empty ip.
  ip4 = (lagopus_ip_address_t *) calloc(1, (sizeof(lagopus_ip_address_t)));
  TEST_ASSERT_NOT_NULL(ip4);

  // Normal case of getter
  {
    /* sockaddr. */
    rc = lagopus_ip_address_sockaddr_get(ip1, &actual_sockaddr1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET, actual_sockaddr1->sa_family);
    inet_ntop(AF_INET, &((struct sockaddr_in *) actual_sockaddr1)->sin_addr,
              ipv4_addr, INET_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(name1, ipv4_addr);

    /* length of sockaddr. */
    rc = lagopus_ip_address_sockaddr_len_get(ip1, &actual_sockaddr_len);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(sizeof(struct sockaddr_in), actual_sockaddr_len);

    /* sockaddr. */
    rc = lagopus_ip_address_sockaddr_get(ip2, &actual_sockaddr2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET6, actual_sockaddr2->sa_family);
    inet_ntop(AF_INET6, &((struct sockaddr_in6 *) actual_sockaddr2)->sin6_addr,
              ipv6_addr, INET6_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(name2, ipv6_addr);

    /* length of sockaddr. */
    rc = lagopus_ip_address_sockaddr_len_get(ip2, &actual_sockaddr_len);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(sizeof(struct sockaddr_in6), actual_sockaddr_len);

    /* sockaddr. */
    rc = lagopus_ip_address_sockaddr_get(ip3, &actual_sockaddr3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(AF_INET, actual_sockaddr3->sa_family);
    inet_ntop(AF_INET, &((struct sockaddr_in *) actual_sockaddr3)->sin_addr,
              ipv4_addr, INET_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(name1, ipv4_addr);

    /* length of sockaddr. */
    rc = lagopus_ip_address_sockaddr_len_get(ip3, &actual_sockaddr_len);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(sizeof(struct sockaddr_in), actual_sockaddr_len);
  }

  // Abnormal case of getter
  {
    rc = lagopus_ip_address_sockaddr_get(NULL, &actual_sockaddr1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = lagopus_ip_address_sockaddr_get(ip1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = lagopus_ip_address_sockaddr_get(ip4, &actual_sockaddr4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = lagopus_ip_address_sockaddr_len_get(NULL, &actual_sockaddr_len);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = lagopus_ip_address_sockaddr_len_get(ip1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free(actual_sockaddr1);
  free(actual_sockaddr2);
  free(actual_sockaddr3);
  free(actual_sockaddr4);

  lagopus_ip_address_destroy(ip1);
  lagopus_ip_address_destroy(ip2);
  lagopus_ip_address_destroy(ip3);
  free(ip4);
}
