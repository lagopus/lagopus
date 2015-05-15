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
#include "lagopus_apis.h"
#include "lagopus/session.h"
#include "lagopus/session_internal.h"
#include "lagopus/session_tls.h"

int s4 = -1, s6 = -1;
void
setUp(void) {
  struct sockaddr_storage so = {0,0,{0}};
  struct sockaddr_in *sin = (struct sockaddr_in *)&so;
  struct sockaddr_in6 *sin6;

  if (s4 != -1) {
    return;
  }

  sin->sin_family = AF_INET;
  sin->sin_port = htons(10022);
  sin->sin_addr.s_addr = INADDR_ANY;

  s4 = socket(AF_INET, SOCK_STREAM, 0);
  if (s4 < 0) {
    perror("socket");
    exit(1);
  }

  if (bind(s4, (struct sockaddr *) sin, sizeof(*sin)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(s4, 5) < 0) {
    perror("listen");
    exit(1);
  }

  sin6 = (struct sockaddr_in6 *)&so;
  sin6->sin6_family = AF_INET6;
  sin6->sin6_port = htons(10023);
  sin6->sin6_addr = in6addr_any;

  s6 = socket(AF_INET6, SOCK_STREAM, 0);
  if (s6 < 0) {
    perror("socket");
    exit(1);
  }

  if (bind(s6, (struct sockaddr *) sin6, sizeof(*sin6)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(s6, 5) < 0) {
    perror("listen");
    exit(1);
  }

}
void
tearDown(void) {
}

void
test_session_create_and_close(void) {
  struct session *ses;

  ses = session_create(SESSION_TCP);
  TEST_ASSERT_NOT_NULL(ses);
  session_destroy(ses);
}

void
test_session_tcp(void) {
  ssize_t ret;
  int sock;
  socklen_t size;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  struct sockaddr_in sin;
  struct session *ses;
  struct addrunion dst;

  ses = session_create(SESSION_TCP);
  TEST_ASSERT_NOT_NULL(ses);

  addrunion_ipv4_set(&dst, "127.0.0.1");
  ret = session_connect(ses, &dst, 10022, NULL, 0);
  if (ret == 0 || errno == EINPROGRESS)  {
    TEST_ASSERT(true);
  } else {
    TEST_ASSERT(false);
  }

  size = sizeof(sin);
  sock = accept(s4, (struct sockaddr *)&sin, &size);
  if (sock < 0) {
    perror("accept");
    close(sock);
  }
  TEST_ASSERT_NOT_EQUAL(-1, sock);
  TEST_ASSERT_TRUE(session_is_alive(ses));

  snprintf(cbuf, sizeof(cbuf), "hogehoge\n");
  ret = session_write(ses, cbuf, strlen(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));
  ret = read(sock, sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  ret = write(sock, sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_read(ses, cbuf, sizeof(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));
  session_close(ses);
  TEST_ASSERT_FALSE(session_is_alive(ses));

  session_destroy(ses);
  ses = NULL;
  TEST_ASSERT_FALSE(session_is_alive(ses));
  close(sock);
}

void
test_session_tcp6(void) {
  ssize_t ret;
  int sock;
  socklen_t size;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  struct sockaddr_in6 sin;
  struct session *ses;
  struct addrunion dst;

  ses = session_create(SESSION_TCP6);
  TEST_ASSERT_NOT_NULL(ses);

  addrunion_ipv6_set(&dst, "::1");
  ret = session_connect(ses, &dst, 10023, NULL, 0);
  if (ret == 0 || errno == EINPROGRESS)  {
    TEST_ASSERT(true);
  } else {
    TEST_ASSERT(false);
  }

  size = sizeof(sin);
  sock = accept(s6, (struct sockaddr *)&sin, &size);
  if (sock < 0) {
    perror("accept");
    close(sock);
  }
  TEST_ASSERT_NOT_EQUAL(-1, sock);

  snprintf(cbuf, sizeof(cbuf), "hogehoge\n");
  ret = session_write(ses, cbuf, strlen(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));
  ret = read(sock, sbuf, sizeof(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));

  ret = write(sock, sbuf, strlen(sbuf));
  TEST_ASSERT_EQUAL(ret, strlen(sbuf));
  ret = session_read(ses, cbuf, sizeof(cbuf));
  TEST_ASSERT_EQUAL(ret, strlen(cbuf));

  session_destroy(ses);
  close(sock);
}

static lagopus_result_t
hoge(const char *a, const char *b) {
  (void) a; (void) b;
  return LAGOPUS_RESULT_OK;
}

void
test_session_tls_set_get(void) {
  const char *s = "hogehoge";
  char *a = NULL;
  session_tls_initialize(hoge);
  session_tls_ca_dir_get(&a);
  session_tls_ca_dir_set(NULL);
  session_tls_ca_dir_set(s);
  session_tls_ca_dir_get(&a);
  TEST_ASSERT_EQUAL_STRING(s, a);

  free(a); a = NULL;
  session_tls_cert_get(&a);
  session_tls_cert_set(NULL);
  session_tls_cert_set(s);
  session_tls_cert_get(&a);
  TEST_ASSERT_EQUAL_STRING(s, a);

  free(a); a = NULL;
  session_tls_key_get(&a);
  session_tls_key_set(NULL);
  session_tls_key_set(s);
  session_tls_key_get(&a);
  TEST_ASSERT_EQUAL_STRING(s, a);
}


void
test_session_tls(void) {
#if 0
  ssize_t ret;
  lagopus_result_t res;
  int sock;
  socklen_t size;
  char cbuf[256] = {0};
  char sbuf[256] = {0};
  struct sockaddr_in sin;
#endif
  struct session *ses;
  struct addrunion src, dst;

  addrunion_ipv4_set(&src, "0.0.0.0");
  addrunion_ipv4_set(&dst, "127.0.0.1");
  ses = session_create(SESSION_TLS);
  TEST_ASSERT_NOT_NULL(ses);

#if 0
  ret = session_connect(ses, &dst, 10022, &src, 0);
  if (ret == 0 || errno == EINPROGRESS)  {
    TEST_ASSERT(true);
  } else {
    TEST_ASSERT(false);
  }

  size = sizeof(sin);
  sock = accept(s4, (struct sockaddr *)&sin, &size);
  if (sock < 0) {
    perror("accept");
    close(sock);
  }
  TEST_ASSERT_NOT_EQUAL(-1, sock);
  TEST_ASSERT_TRUE(session_is_alive(ses));

  res = session_connect_check(ses);
  TEST_ASSERT_EQUAL(res, LAGOPUS_RESULT_EINPROGRESS);

  session_destroy(ses);
  ses = NULL;
  TEST_ASSERT_FALSE(session_is_alive(ses));
#endif
}
