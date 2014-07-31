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


#include "lagopus_apis.h"
#include "lagopus/session.h"
#include "lagopus/session_internal.h"
#include "lagopus/session_tls.h"
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

struct session *session_tls_init(struct session *);
static lagopus_result_t connect_tls(struct session *s, const char *host,
                                    const char *port);
static void destroy_tls(struct session *s);
static void close_tls(struct session *s);
static ssize_t read_tls(struct session *s, void *buf, size_t n);
static ssize_t write_tls(struct session *s, void *buf, size_t n);
static lagopus_result_t connect_check_tls(struct session *s);

void
session_tls_close(struct session *s);

static lagopus_mutex_t lock = NULL;
static pthread_once_t initialized = PTHREAD_ONCE_INIT;

static char *client_ca_dir = NULL;
static char *client_cert = NULL;
static char *client_key = NULL;
static lagopus_result_t
(*check_certificates)(const char *issuer_dn, const char *subject_dn) = NULL;

#define GET_TLS_CTX(a)  ((struct tls_ctx *)((a)->ctx))
#define IS_CTX_NULL(a)  ((a)->ctx == NULL)
#define IS_TLS_NOT_INIT(a)  (GET_TLS_CTX(a)->ctx == NULL)

struct tls_ctx {
  char *ca_dir;
  char *cert;
  char *key;
  SSL *ssl;
  SSL_CTX *ctx;
  bool verified;
};

static ssize_t
read_tls(struct session *s, void *buf, size_t n) {
  int ret;

  if (IS_CTX_NULL(s)) {
    lagopus_msg_warning("session ctx is null.\n");
    return -1;
  }

  do {
    ret = SSL_read(GET_TLS_CTX(s)->ssl, buf, (int) n);
  } while (SSL_pending(GET_TLS_CTX(s)->ssl));

  return (ssize_t) ret;
}

static ssize_t
write_tls(struct session *s, void *buf, size_t n) {
  int ret;

  if (IS_CTX_NULL(s)) {
    lagopus_msg_warning("session ctx is null.\n");
    return -1;
  }

  ret = SSL_write(GET_TLS_CTX(s)->ssl, buf, (int) n);
  if (SSL_get_error(GET_TLS_CTX(s)->ssl, ret) == SSL_ERROR_WANT_WRITE) {
    /* wrote but blocked. */
    ret = 0;
  }
  return ret;
}

static int verify_callback(int ok, X509_STORE_CTX *store) {
  (void) store;
  return ok;
}

static SSL_CTX *get_ssl_ctx(char *ca_dir, char *cert, char *key) {
  SSL_CTX *ssl_ctx;
  const SSL_METHOD *method;

  lagopus_msg_info("ca_dir:[%s], cert:[%s], key:[%s]\n", ca_dir, cert, key);
  method = TLSv1_method();
  ssl_ctx = SSL_CTX_new(method);
  if (ssl_ctx == NULL) {
    lagopus_msg_warning("no memory.\n");
    return NULL;
  }

  /* add client cert. */
  if (SSL_CTX_use_certificate_file(ssl_ctx, cert, SSL_FILETYPE_PEM) != 1) {
    lagopus_msg_warning("SSL_CTX_use_certificate_file(%s) fail.\n", cert);
    SSL_CTX_free(ssl_ctx);
    return NULL;
  }

  /* add client private key. */
  if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key, SSL_FILETYPE_PEM) != 1) {
    lagopus_msg_warning("SSL_CTX_use_PrivateKey_file(%s) fail.\n", key);
    SSL_CTX_free(ssl_ctx);
    return NULL;
  }

  /* loading ca cert */
  if (SSL_CTX_load_verify_locations(ssl_ctx, NULL, ca_dir) != 1) {
    lagopus_msg_warning("SSL_CTX_load_verify_locations(%s) fail.\n", ca_dir);
    SSL_CTX_free(ssl_ctx);
    return NULL;
  }

  SSL_CTX_set_verify_depth(ssl_ctx, 1);/* XXX depth is configurable? */
  SSL_CTX_set_verify(ssl_ctx,
                     SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);

  return ssl_ctx;
}

static void
initialize_internal(void) {
  lagopus_result_t ret;

  ret = lagopus_mutex_create(&lock);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("session_tls_init:lagopus_mutex_create");
  }

  SSL_library_init();
  SSL_load_error_strings();
}

void
session_tls_ca_dir_set(const char *c) {
  if (c == NULL) {
    return;
  }
  lagopus_mutex_lock(&lock);
  free(client_ca_dir);
  client_ca_dir = strdup(c);
  lagopus_mutex_unlock(&lock);
}

void
session_tls_ca_dir_get(char **c) {
  lagopus_mutex_lock(&lock);
  if (client_ca_dir == NULL) {
    *c = NULL;
  } else {
    *c = strdup(client_ca_dir);
  }
  lagopus_mutex_unlock(&lock);
}

void
session_tls_cert_set(const char *c) {
  if (c == NULL) {
    return;
  }
  lagopus_mutex_lock(&lock);
  free(client_cert);
  client_cert = strdup(c);
  lagopus_mutex_unlock(&lock);
}

void
session_tls_cert_get(char **c) {
  lagopus_mutex_lock(&lock);
  if (client_cert == NULL) {
    *c = NULL;
  } else {
    *c = strdup(client_cert);
  }
  lagopus_mutex_unlock(&lock);
}

void
session_tls_key_set(const char *c) {
  if (c == NULL) {
    return;
  }
  lagopus_mutex_lock(&lock);
  free(client_key);
  client_key = strdup(c);
  lagopus_mutex_unlock(&lock);
}

void
session_tls_key_get(char **c) {
  lagopus_mutex_lock(&lock);
  if (client_key == NULL) {
    *c = NULL;
  } else {
    *c = strdup(client_key);
  }
  lagopus_mutex_unlock(&lock);
}

void
session_tls_certcheck_set(lagopus_result_t
                          (*func)(const char *, const char *)) {
  lagopus_mutex_lock(&lock);
  check_certificates = func;
  lagopus_mutex_unlock(&lock);
}

void
session_tls_initialize(lagopus_result_t
                       (*func)(const char *, const char *)) {
  pthread_once(&initialized, initialize_internal);
  session_tls_certcheck_set(func);
}

struct session *
session_tls_init(struct session *s) {
  pthread_once(&initialized, initialize_internal);

  s->ctx = malloc(sizeof(struct tls_ctx));
  if (s->ctx == NULL) {
    lagopus_msg_warning("no memory.\n");
    return NULL;
  }

  lagopus_mutex_lock(&lock);
  if (client_ca_dir == NULL || client_ca_dir == NULL
      || client_key == NULL) {
    lagopus_msg_warning("certificate files not initilized.\n");
    lagopus_mutex_unlock(&lock);
    free(s->ctx);
    s->ctx = NULL;
    return NULL;
  }

  GET_TLS_CTX(s)->ca_dir = strdup(client_ca_dir);
  GET_TLS_CTX(s)->cert = strdup(client_cert);
  GET_TLS_CTX(s)->key = strdup(client_key);
  lagopus_mutex_unlock(&lock);

  GET_TLS_CTX(s)->ctx = NULL;
  GET_TLS_CTX(s)->ssl = NULL;
  GET_TLS_CTX(s)->verified = false;
  s->read = read_tls;
  s->write = write_tls;
  s->connect = connect_tls;
  s->close = close_tls;
  s->destroy = destroy_tls;
  s->connect_check = connect_check_tls;

  return s;
}

static int
check_cert_chain(const SSL *ssl) {
  int ret = -1;
  char *issuer = NULL, *subject = NULL;
  X509 *peer = NULL;

  if (SSL_get_verify_result(ssl) != X509_V_OK) {
    lagopus_msg_warning("SSL_get_verify_result() failed.\n");
    return -1;
  }

  peer = SSL_get_peer_certificate(ssl);
  if (peer == NULL) {
    lagopus_msg_warning("SSL_get_peer_certificate() failed.\n");
    return -1;
  }

  issuer = X509_NAME_oneline(X509_get_issuer_name(peer), NULL, 0);
  if (issuer == NULL) {
    lagopus_msg_warning("no memory, get_issuer_name failed.\n");
    ret = -1;
    goto done;
  }

  subject = X509_NAME_oneline(X509_get_subject_name(peer), NULL, 0);
  if (issuer == NULL) {
    lagopus_msg_warning("no memory, get_subject_name failed.\n");
    ret = -1;
    goto done;
  }

  if (check_certificates != NULL) {
    if (check_certificates(issuer, subject) == LAGOPUS_RESULT_OK) {
      ret = 0;
    } else {
      ret = -1;
    }
    lagopus_msg_info("issuer:[%s], subject:[%s] ret: %d\n", issuer, subject, ret);
  } else {
    lagopus_msg_warning("check_certificates function is null.\n");
  }

done:
  free(issuer);
  free(subject);
  X509_free(peer);

  return ret;
}

static lagopus_result_t
connect_tls(struct session *s, const char *host, const char *port) {
  int ret;
  BIO *sbio;

  (void) host;
  (void) port;

  lagopus_msg_info("tls handshake start.\n");

  if (IS_TLS_NOT_INIT(s)) {
    SSL_CTX *ssl_ctx;

    ssl_ctx = get_ssl_ctx(GET_TLS_CTX(s)->ca_dir, GET_TLS_CTX(s)->cert,
                          GET_TLS_CTX(s)->key);
    if (ssl_ctx == NULL) {
      lagopus_msg_warning("get_ssl_ctx() fail.\n");
      return LAGOPUS_RESULT_TLS_CONN_ERROR;
    }
    GET_TLS_CTX(s)->ctx = ssl_ctx;
  }

  if (GET_TLS_CTX(s)->ssl == NULL) {
    SSL *ssl;
    ssl = SSL_new(GET_TLS_CTX(s)->ctx);
    if (ssl == NULL) {
      lagopus_msg_warning("no memory.\n");
      return LAGOPUS_RESULT_TLS_CONN_ERROR;
    }
    GET_TLS_CTX(s)->ssl = ssl;
  }

  if (SSL_get_rbio(GET_TLS_CTX(s)->ssl) == NULL) {
    sbio = BIO_new_socket(s->sock, BIO_NOCLOSE);
    SSL_set_bio(GET_TLS_CTX(s)->ssl, sbio, sbio);
  }

  ret = SSL_connect(GET_TLS_CTX(s)->ssl);
  if (ret == 0) {
    lagopus_msg_warning("tls handshake failed.\n");
    return LAGOPUS_RESULT_TLS_CONN_ERROR;
  } else if (ret < 0
             && (SSL_get_error(GET_TLS_CTX(s)->ssl, ret) != SSL_ERROR_WANT_READ
                 && SSL_get_error(GET_TLS_CTX(s)->ssl, ret) != SSL_ERROR_WANT_READ)) {
    lagopus_msg_warning("tls error (%s:%d).\n", ERR_error_string((unsigned long)
                        SSL_get_error(GET_TLS_CTX(s)->ssl, ret), NULL),
                        (int) SSL_get_error(GET_TLS_CTX(s)->ssl, ret));
    return LAGOPUS_RESULT_TLS_CONN_ERROR;
  } else if (ret < 0) {
    lagopus_msg_info("tls error (%s:%d), but continue.\n",
                     ERR_error_string((unsigned long)
                                      SSL_get_error(GET_TLS_CTX(s)->ssl, ret), NULL),
                     (int) SSL_get_error(GET_TLS_CTX(s)->ssl, ret));
    return LAGOPUS_RESULT_EINPROGRESS;
  } else {
    ret = check_cert_chain(GET_TLS_CTX(s)->ssl);
    if (ret < 0) {
      lagopus_msg_warning("certificate error.\n");
      return LAGOPUS_RESULT_TLS_CONN_ERROR;
    }
    GET_TLS_CTX(s)->verified = true;
    lagopus_msg_info("tls handshake end.\n");
  }

  return LAGOPUS_RESULT_OK;
}


static void
destroy_tls(struct session *s) {
  if (IS_TLS_NOT_INIT(s) == false) {
    free(GET_TLS_CTX(s)->ca_dir);
    free(GET_TLS_CTX(s)->cert);
    free(GET_TLS_CTX(s)->key);
    SSL_free(GET_TLS_CTX(s)->ssl);
    GET_TLS_CTX(s)->ssl = NULL;
    SSL_CTX_free(GET_TLS_CTX(s)->ctx);
    GET_TLS_CTX(s)->ctx = NULL;
  }

  free(GET_TLS_CTX(s));
  s->ctx =  NULL;
}

static void
close_tls(struct session *s) {
  if (IS_TLS_NOT_INIT(s) == false) {
    SSL_shutdown(GET_TLS_CTX(s)->ssl);
    SSL_clear(GET_TLS_CTX(s)->ssl);
  }
}

static lagopus_result_t
connect_check_tls(struct session *s) {
  long res = -1;
  X509 *peer = NULL;
  lagopus_result_t ret = 0;

  lagopus_msg_debug(10, "connect check in\n");
  if (IS_CTX_NULL(s)) {
    lagopus_msg_warning("session ctx is null.\n");
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (IS_TLS_NOT_INIT(s) == false) {
    res = SSL_get_verify_result(GET_TLS_CTX(s)->ssl);
    peer = SSL_get_peer_certificate(GET_TLS_CTX(s)->ssl);
  }

  if (res != X509_V_OK || peer == NULL
      || GET_TLS_CTX(s)->verified == false) {
    ret = connect_tls(s, NULL, NULL);
  }
  lagopus_msg_debug(10, "connect check out ret:%d\n", (int) ret);

  return ret;
}
