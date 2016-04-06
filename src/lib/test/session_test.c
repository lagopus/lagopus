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
#include "lagopus_session.h"
#include "lagopus_session_tls.h"
#include "../session_internal.h"
#include <sys/stat.h>

int s4 = -1, s6 = -1;
void
setUp(void) {
}

void
tearDown(void) {
}

void
test_session_create_and_close(void) {
  lagopus_result_t ret;
  lagopus_session_t ses;

  ret = session_create(SESSION_TCP|SESSION_ACTIVE, &ses);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  session_destroy(ses);
}

void
test_session_create_and_fail(void) {
  lagopus_result_t ret;
  lagopus_session_t ses;

  ret = session_create(SESSION_ACTIVE|SESSION_TLS, &ses);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
  ret = session_create(SESSION_TCP|SESSION_TLS, &ses);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
  ret = session_create(SESSION_TLS|SESSION_PASSIVE, &ses);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
  ret = session_create(SESSION_TCP|SESSION_ACTIVE, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);

}

void
test_session_tcp(void) {
  lagopus_result_t ret;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  bool b;
  lagopus_session_t sesc, sess, sesa, sesp[3];
  lagopus_ip_address_t *dst, *src;

  ret = session_create(SESSION_TCP|SESSION_PASSIVE, &sess);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("0.0.0.0", true, &src);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_bind(sess, src, 10022);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_create(SESSION_TCP|SESSION_ACTIVE, &sesc);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("127.0.0.1", true, &dst);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_connect(sesc, dst, 10022, NULL, 0);
  if (ret == 0 || errno == EINPROGRESS)  {
    TEST_ASSERT(true);
  } else {
    TEST_ASSERT(false);
  }

  session_write_event_set(sesc);
  session_read_event_set(sess);
  sesp[0] = sesc;
  sesp[1] = sess;
  ret = session_poll(sesp, 2, 1);
  TEST_ASSERT_EQUAL(2, ret);

  ret = session_is_writable(sesc, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_TRUE(b);

  ret = session_is_readable(sess, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_TRUE(b);

  ret = session_accept(sess, &sesa);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_NOT_NULL(sesa);

  TEST_ASSERT_TRUE(session_is_alive(sess));
  TEST_ASSERT_TRUE(session_is_alive(sesc));
  TEST_ASSERT_TRUE(session_is_alive(sesa));

  session_write_event_set(sesc);
  session_read_event_set(sess);
  session_read_event_set(sesa);
  sesp[0] = sesc;
  sesp[1] = sess;
  sesp[2] = sesa;
  ret = session_poll(sesp, 3, 1);
  TEST_ASSERT_EQUAL(1, ret);

  ret = session_is_writable(sesc, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_TRUE(b);

  ret = session_is_readable(sess, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_FALSE(b);

  ret = session_is_readable(sesa, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_FALSE(b);

  snprintf(cbuf, sizeof(cbuf), "hogehoge\n");
  ret = session_write(sesc, cbuf, strlen(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));

  session_read_event_set(sesa);
  sesp[0] = sesa;
  ret = session_poll(sesp, 1, 1);
  TEST_ASSERT_EQUAL(1, ret);

  ret = session_is_readable(sesa, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_TRUE(b);

  ret = session_read(sesa, sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_write(sesa, sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_read(sesc, cbuf, sizeof(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));

  session_close(sesc);
  TEST_ASSERT_FALSE(session_is_alive(sesc));

  session_close(sesa);
  TEST_ASSERT_FALSE(session_is_alive(sesa));

  session_close(sess);
  TEST_ASSERT_FALSE(session_is_alive(sesa));

  session_write_event_set(sesc);
  session_read_event_set(sess);
  session_read_event_set(sesa);
  sesp[0] = sesc;
  sesp[1] = sess;
  sesp[2] = sesa;
  ret = session_poll(sesp, 3, 1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TIMEDOUT, ret);

  ret = session_is_writable(sesc, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_FALSE(b);

  ret = session_is_readable(sess, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_FALSE(b);

  ret = session_is_readable(sesa, &b);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_FALSE(b);

  lagopus_ip_address_destroy(dst);
  lagopus_ip_address_destroy(src);
  session_destroy(sesc);
  session_destroy(sesa);
  session_destroy(sess);
}

void
test_session_tcp6(void) {
  lagopus_result_t ret;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  lagopus_session_t sesc, sess, sesa;
  lagopus_ip_address_t *dst, *src;

  ret = session_create(SESSION_TCP6|SESSION_PASSIVE, &sess);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("::0", false, &src);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_bind(sess, src, 10023);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_create(SESSION_TCP6|SESSION_ACTIVE, &sesc);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("::1", false, &dst);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_connect(sesc, dst, 10023, NULL, 0);
  if (ret == 0 || errno == EINPROGRESS)  {
    TEST_ASSERT(true);
  } else {
    TEST_ASSERT(false);
  }

  ret = session_accept(sess, &sesa);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_NOT_NULL(sesa);

  TEST_ASSERT_TRUE(session_is_alive(sess));
  TEST_ASSERT_TRUE(session_is_alive(sesc));
  TEST_ASSERT_TRUE(session_is_alive(sesa));

  snprintf(cbuf, sizeof(cbuf), "hogehoge\n");
  ret = session_write(sesc, cbuf, strlen(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));
  ret = session_read(sesa, sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  ret = session_write(sesa, sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_read(sesc, cbuf, sizeof(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));

  lagopus_ip_address_destroy(dst);
  lagopus_ip_address_destroy(src);
  session_destroy(sesc);
  session_destroy(sesa);
  session_destroy(sess);
}

void
test_session_pair_dgram(void) {
  lagopus_result_t ret;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  lagopus_session_t s[2];

  ret = session_pair(SESSION_UNIX_DGRAM, s);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  snprintf(cbuf, sizeof(cbuf), "hogehoge\n");
  ret = session_write(s[0], cbuf, strlen(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));
  ret = session_read(s[1], sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  ret = session_write(s[1], sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_read(s[0], cbuf, sizeof(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));

  session_destroy(s[0]);
  session_destroy(s[1]);
}

void
test_session_pair_stream(void) {
  lagopus_result_t ret;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  lagopus_session_t s[2];

  ret = session_pair(SESSION_UNIX_STREAM, s);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  snprintf(cbuf, sizeof(cbuf), "hogehoge\n");
  ret = session_write(s[0], cbuf, strlen(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));
  ret = session_read(s[1], sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  ret = session_write(s[1], sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_read(s[0], cbuf, sizeof(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));

  session_destroy(s[0]);
  session_destroy(s[1]);
}

/*
 * Cannot do unit-tests for initialization of session_tls
 * so that lagopus_session_tls is not included in this file.
 *
void
test_session_tls_set_get(void) {
  const char *s1 = CONFDIR;
  const char *s2 = CONFDIR "/TEST_SESSION_TLS_FILE1";
  FILE *fp = NULL;
  fp = fopen(s2, "w");
  if (fp != NULL) {
    fclose(fp);
  }

  char *a = NULL;
  lagopus_session_tls_get_ca_dir(&a);
  lagopus_session_tls_set_ca_dir(NULL);
  lagopus_session_tls_set_ca_dir(s1);
  lagopus_session_tls_get_ca_dir(&a);
  TEST_ASSERT_EQUAL_STRING(s1, a);

  free(a); a = NULL;
  lagopus_session_tls_get_client_cert(&a);
  lagopus_session_tls_set_client_cert(NULL);
  lagopus_session_tls_set_client_cert(s2);
  lagopus_session_tls_get_client_cert(&a);
  TEST_ASSERT_EQUAL_STRING(s2, a);

  free(a); a = NULL;
  lagopus_session_tls_get_server_cert(&a);
  lagopus_session_tls_set_server_cert(NULL);
  lagopus_session_tls_set_server_cert(s2);
  lagopus_session_tls_get_server_cert(&a);
  TEST_ASSERT_EQUAL_STRING(s2, a);

  free(a); a = NULL;
  lagopus_session_tls_get_client_key(&a);
  lagopus_session_tls_set_client_key(NULL);
  lagopus_session_tls_set_client_key(s2);
  lagopus_session_tls_get_client_key(&a);
  TEST_ASSERT_EQUAL_STRING(s2, a);

  free(a); a = NULL;
  lagopus_session_tls_get_server_key(&a);
  lagopus_session_tls_set_server_key(NULL);
  lagopus_session_tls_set_server_key(s2);
  lagopus_session_tls_get_server_key(&a);
  TEST_ASSERT_EQUAL_STRING(s2, a);

  free(a); a = NULL;
  lagopus_session_tls_get_point_conf(&a);
  lagopus_session_tls_set_trust_point_conf(NULL);
  lagopus_session_tls_set_trust_point_conf(s2);
  lagopus_session_tls_get_point_conf(&a);
  TEST_ASSERT_EQUAL_STRING(s2, a);

  free(a);
  unlink(s2);
}
*/


void
test_session_tls(void) {
#if 0 /* this test code is not work, use openssl s_server/s_client commands for tls session tests. */
  lagopus_result_t ret;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  lagopus_session_t sesc, sess, sesa;
  lagopus_ip_address_t *src, *dst;

  session_tls_set_ca_dir("ca");
  session_tls_set_server_cert("./server1.pem");
  session_tls_set_server_key("./server1_key_nopass.pem");
  session_tls_set_client_key("./client1_key_nopass.pem");

  ret = session_create(SESSION_TCP|SESSION_PASSIVE|SESSION_TLS, &sess);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("0.0.0.0", true, &src);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_bind(sess, src, 10024);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("127.0.0.1", true, &dst);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_create(SESSION_TCP|SESSION_TLS|SESSION_ACTIVE, &sesc);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_connect(sesc, dst, 10024, &dst, 0);
  if (ret == 0 || errno == EINPROGRESS)  {
    TEST_ASSERT(true);
  } else {
    TEST_ASSERT(false);
  }

  ret = session_accept(sess, &sesa);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_NOT_NULL(sesa);

  TEST_ASSERT_TRUE(session_is_alive(sess));
  TEST_ASSERT_TRUE(session_is_alive(sesa));

  ret = session_read(sesa, sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  ret = session_write(sesa, sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  TEST_ASSERT_TRUE(session_is_alive(sesc));

  snprintf(cbuf, sizeof(cbuf), "hogehoge\n");
  ret = session_write(sesc, cbuf, strlen(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));
  ret = session_read(sesa, sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  ret = session_write(sesa, sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_read(sesc, cbuf, sizeof(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));

  lagopus_ip_address_destroy(dst);
  lagopus_ip_address_destroy(src);
  session_destroy(sesc);
  session_destroy(sesa);
  session_destroy(sess);
#endif
}

void
test_session_fgets(void) {
  ssize_t ret;
  char *c;
#if 0
  char cbuf[256] = {0};
#endif
  char sbuf[256] = {0};
  lagopus_session_t sesc, sess, sesa, sesp[1];
  lagopus_ip_address_t *dst, *src;

  ret = session_create(SESSION_TCP|SESSION_PASSIVE, &sess);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("0.0.0.0", true, &src);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_bind(sess, src, 10022);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_create(SESSION_TCP|SESSION_ACTIVE, &sesc);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = lagopus_ip_address_create("127.0.0.1", true, &dst);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = session_connect(sesc, dst, 10022, NULL, 0);
  if (ret == 0 || errno == EINPROGRESS)  {
    TEST_ASSERT(true);
  } else {
    TEST_ASSERT(false);
  }

  ret = session_accept(sess, &sesa);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_NOT_NULL(sesa);

  TEST_ASSERT_TRUE(session_is_alive(sess));
  TEST_ASSERT_TRUE(session_is_alive(sesc));
  TEST_ASSERT_TRUE(session_is_alive(sesa));

  ret = session_printf(sesc, "%s", "hogehoge\n\nhoge");
  TEST_ASSERT_EQUAL(ret, strlen("hogehoge\n\nhoge"));

  session_read_event_set(sesa);
  sesp[0] = sesa;
  ret = session_poll(sesp, 1, 1);
  TEST_ASSERT_EQUAL(ret, 1);

  c = session_fgets(sbuf, 5, sesa);
  fprintf(stderr, "1.%s\n", sbuf);
  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL(0, strncmp(sbuf, "hoge", 5));

  c = session_fgets(sbuf, 10, sesa);
  fprintf(stderr, "2.%s\n", sbuf);
  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL(0, strncmp(sbuf, "hoge\n", 5));

  c = session_fgets(sbuf, 10, sesa);
  fprintf(stderr, "3.%s\n", sbuf);
  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL(0, strncmp(sbuf, "\n", 1));

  c = session_fgets(sbuf, 5, sesa);
  fprintf(stderr, "4.%s\n", sbuf);
  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL(0, strncmp(sbuf, "hoge", 5));

#if 0
  do {
    fprintf(stderr, "out:%s", sbuf);
    fgets(cbuf, sizeof(cbuf), stdin);
    fprintf(stderr, "in :%s", cbuf);
    session_write(sesc, cbuf, strlen(cbuf));
  } while (session_fgets(sbuf, sizeof(sbuf), sesa) != NULL);
#endif

  session_close(sesc);
  TEST_ASSERT_FALSE(session_is_alive(sesc));
  session_close(sesa);
  TEST_ASSERT_FALSE(session_is_alive(sesa));
  session_close(sess);
  TEST_ASSERT_FALSE(session_is_alive(sesa));

  lagopus_ip_address_destroy(dst);
  lagopus_ip_address_destroy(src);
  session_destroy(sesc);
  session_destroy(sesa);
  session_destroy(sess);
}
