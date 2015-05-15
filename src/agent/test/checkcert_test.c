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


#include "lagopus_apis.h"
#include "../checkcert.h"
#include "unity.h"





static pthread_once_t s_init_once = PTHREAD_ONCE_INIT;
static pthread_once_t s_final_once = PTHREAD_ONCE_INIT;
static bool s_last = false;


static void
s_init(void) {
  agent_check_certificates_initialize();
}


static void
s_final(void) {
  agent_check_certificates_finalize();
}





void
setUp(void) {
  (void)pthread_once(&s_init_once, s_init);
}


void
tearDown(void) {
  if (s_last == true) {
    (void)pthread_once(&s_final_once, s_final);
  }
}





void
test_load_failure(void) {
  lagopus_result_t rc;

  agent_check_certificates_set_config_file("./not yet exists.");
  rc = agent_check_certificates("aaa", "bbb");
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_NOT_FOUND);
}


void
test_load(void) {
  lagopus_result_t rc;

  agent_check_certificates_set_config_file("./test-load.conf");

  rc = agent_check_certificates("aaa", "bbb");
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_NOT_ALLOWED);
}


void
test_deny_selfsigned(void) {
  lagopus_result_t rc = agent_check_certificates("same", "same");
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_NOT_ALLOWED);
}


void
test_match_full(void) {
  const char *issuer =
    "/C=JP/ST=Somewhere/L=Somewhere/O=Somewhere/OU=Somewhere/CN=Middle of Nowhere's tier I CA/emailAddress=ca@example.net";
  const char *subject =
    "/C=JP/ST=Somewhere/L=Somewhere/O=Somewhere/OU=Somewhere/CN=John \"774\" Doe/emailAddress=john.774.doe@example.net";

  lagopus_result_t rc = agent_check_certificates(issuer, subject);
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_OK);
}


void
test_match_regex_allow_issuer(void) {
  lagopus_result_t rc = agent_check_certificates("aaa/CN=The Evil Genius CA",
                        NULL);
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_OK);
}


void
test_match_regex_deny_subject(void) {
  lagopus_result_t rc;

  agent_check_certificates_set_config_file("./test-regex.conf");

  rc = agent_check_certificates(NULL, "aaa/CN=The Evil Genius hehehe");
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_NOT_ALLOWED);
}


void
test_match_regex_allow_self_signed(void) {
  lagopus_result_t rc;
  const char *self = "/C=JP/CN=John \"774\" Doe";

  rc = agent_check_certificates(self, self);
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_OK);

  rc = agent_check_certificates(self, NULL);
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_NOT_ALLOWED);

  rc = agent_check_certificates(NULL, self);
  TEST_ASSERT_EQUAL(rc, LAGOPUS_RESULT_NOT_ALLOWED);
}


void
test_this_must_be_last(void) {
  s_last = true;
}
