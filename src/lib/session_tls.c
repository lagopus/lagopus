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
#include "lagopus_session.h"
#include "lagopus_session_tls.h"
#include "session_internal.h"
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <errno.h>
#include <stdlib.h>

static void	s_ctors(void) __attr_constructor__(111);
static void	s_dtors(void) __attr_destructor__(111);

lagopus_result_t session_tls_init(lagopus_session_t s);
static lagopus_result_t connect_tls(lagopus_session_t s, const char *host,
                                    const char *port);
static void destroy_tls(lagopus_session_t s);
static void close_tls(lagopus_session_t s);
static ssize_t read_tls(lagopus_session_t s, void *buf, size_t n);
static ssize_t write_tls(lagopus_session_t s, void *buf, size_t n);
static lagopus_result_t connect_check_tls(lagopus_session_t s);
static int check_cert_chain(const lagopus_session_t s);

void
session_tls_close(lagopus_session_t s);

static lagopus_result_t
valid_path(const char *path, const bool is_file);

static lagopus_mutex_t lock = NULL;
static pthread_once_t initialized = PTHREAD_ONCE_INIT;

static lagopus_result_t
(*check_certificates_default)(const char *issuer_dn,
                              const char *subject_dn) = NULL;
static int server_session_id_context = 1;

#define GET_TLS_CTX(a)  ((struct tls_ctx *)((a)->ctx))
#define IS_CTX_NULL(a)  ((a)->ctx == NULL)
#define IS_TLS_NOT_INIT(a)  (GET_TLS_CTX(a)->ctx == NULL)

struct tls_ctx {
  char *ca_dir;
  char *cert;
  char *key;
  SSL *ssl;
  SSL_CTX *ctx;
  lagopus_result_t
  (*check_certificates)(const char *issuer_dn, const char *subject_dn);
  bool verified;
};

typedef struct tls_conf {
  char session_cert[PATH_MAX];
  char private_key[PATH_MAX];
  char session_ca_dir[PATH_MAX];
  lagopus_rwlock_t s_lck;
} tls_conf_t;

static tls_conf_t tls;

#include "session_checkcert.c"

static ssize_t
read_tls(lagopus_session_t s, void *buf, size_t n) {
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
write_tls(lagopus_session_t s, void *buf, size_t n) {
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

static SSL_CTX *
get_ssl_ctx_common(char *ca_dir, char *cert, char *key) {
  int ret;
  SSL_CTX *ssl_ctx;
  const SSL_METHOD *method;

  lagopus_msg_info("ca_dir:[%s], cert:[%s], key:[%s]\n", ca_dir, cert, key);
  method = TLSv1_method();
  ssl_ctx = SSL_CTX_new(method);
  if (ssl_ctx == NULL) {
    lagopus_msg_warning("no memory.\n");
    return NULL;
  }

  /* add cert. */
  ret = SSL_CTX_use_certificate_file(ssl_ctx, cert, SSL_FILETYPE_PEM);
  if (ret != 1) {
    unsigned long n = ERR_get_error();
    lagopus_msg_warning("SSL_CTX_use_certificate_file(%s):error (%s:%d).\n", cert,
                        ERR_error_string(n, NULL), (int) n);
    SSL_CTX_free(ssl_ctx);
    return NULL;
  }

  /* add private key. */
  ret = SSL_CTX_use_PrivateKey_file(ssl_ctx, key, SSL_FILETYPE_PEM);
  if (ret != 1) {
    unsigned long n = ERR_get_error();
    lagopus_msg_warning("SSL_CTX_use_PrivateKey_file(%s):error (%s:%d).\n", key,
                        ERR_error_string(n, NULL), (int) n);
    SSL_CTX_free(ssl_ctx);
    return NULL;
  }

  /* loading ca cert */
  if (SSL_CTX_load_verify_locations(ssl_ctx, NULL, ca_dir) != 1) {
    lagopus_msg_warning("SSL_CTX_load_verify_locations(%s) fail.\n", ca_dir);
    SSL_CTX_free(ssl_ctx);
    return NULL;
  }

  return ssl_ctx;
}

static SSL_CTX *
get_server_ssl_ctx(char *ca_dir, char *cert, char *key) {
  SSL_CTX *ssl_ctx;

  ssl_ctx = get_ssl_ctx_common(ca_dir, cert, key);
  if (ssl_ctx == NULL) {
    return NULL;
  }

  SSL_CTX_set_verify_depth(ssl_ctx, 1);/* XXX depth is configurable? */
  SSL_CTX_set_verify(ssl_ctx,
                     SSL_VERIFY_PEER, verify_callback);

  /* XXX load dh param */
  /* XXX generate rsa key */

  SSL_CTX_set_session_id_context(ssl_ctx,
                                 (void *) &server_session_id_context,
                                 sizeof(server_session_id_context));

  return ssl_ctx;
}

static SSL_CTX *
get_client_ssl_ctx(char *ca_dir, char *cert, char *key) {
  SSL_CTX *ssl_ctx;

  ssl_ctx = get_ssl_ctx_common(ca_dir, cert, key);
  if (ssl_ctx == NULL) {
    return NULL;
  }

  SSL_CTX_set_verify_depth(ssl_ctx, 1);/* XXX depth is configurable? */
  SSL_CTX_set_verify(ssl_ctx,
                     SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);

  return ssl_ctx;
}

static lagopus_result_t
accept_tls(lagopus_session_t s1, lagopus_session_t *s2) {
  int ret;
  SSL *ssl;
  BIO *sbio;

  if (s1 == NULL || *s2 == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  ssl = SSL_new(GET_TLS_CTX(s1)->ctx);
  if (ssl == NULL) {
    lagopus_msg_warning("no memory.\n");
    return LAGOPUS_RESULT_TLS_CONN_ERROR;
  }
  GET_TLS_CTX(*s2)->ssl = ssl;

  sbio = BIO_new_socket((*s2)->sock, BIO_NOCLOSE);
  SSL_set_bio(GET_TLS_CTX(*s2)->ssl, sbio, sbio);

retry:
  ret = SSL_accept(GET_TLS_CTX(*s2)->ssl);
  if (ret <= 0) {
    if (SSL_get_error(GET_TLS_CTX(*s2)->ssl, ret) == SSL_ERROR_WANT_READ
        || SSL_get_error(GET_TLS_CTX(*s2)->ssl, ret) == SSL_ERROR_WANT_WRITE) {
      goto retry;
    }
    lagopus_msg_warning("tls error (%s:%d).\n", ERR_error_string((unsigned long)
                        SSL_get_error(GET_TLS_CTX(*s2)->ssl, ret), NULL),
                        (int) SSL_get_error(GET_TLS_CTX(*s2)->ssl, ret));
    destroy_tls(*s2);
    return LAGOPUS_RESULT_TLS_CONN_ERROR;
  }

  if (SSL_get_peer_certificate(GET_TLS_CTX(*s2)->ssl) != NULL) {
    ret = check_cert_chain(*s2);
    if (ret < 0) {
      lagopus_msg_warning("certificate error.\n");
      destroy_tls(*s2);
      return LAGOPUS_RESULT_TLS_CONN_ERROR;
    }
    GET_TLS_CTX(*s2)->verified = true;
  }

  return LAGOPUS_RESULT_OK;
}

static void
initialize_internal(void) {
  lagopus_result_t ret;

  ret = lagopus_mutex_create(&lock);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("session_tls_init:lagopus_mutex_create");
  }

  tls.s_lck = NULL;
  if ((ret = lagopus_rwlock_create(&(tls.s_lck))) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_exit_fatal("can't initialize the configurator lock.\n");
  }

  SSL_library_init();
  SSL_load_error_strings();

  s_checkcert_init();

  lagopus_session_tls_set_certcheck_default(s_check_certificates);
}

lagopus_result_t
lagopus_session_tls_set_ca_dir(const char *new_certstore) {
  if (IS_VALID_STRING(new_certstore) == true) {
    if (strlen(new_certstore) < PATH_MAX) {
      (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
      {
        snprintf(tls.session_ca_dir, sizeof(tls.session_ca_dir),
                 "%s", new_certstore);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
lagopus_session_tls_get_ca_dir(char **certstore) {
  if (certstore != NULL && *certstore == NULL) {
    if (IS_VALID_STRING(tls.session_ca_dir) == true) {
      char *str = NULL;

      (void)lagopus_rwlock_reader_lock(&(tls.s_lck));
      {
        str = strdup(tls.session_ca_dir);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));

      if (str != NULL) {
        *certstore = str;
        return LAGOPUS_RESULT_OK;
      }
      free((void *)str);
      return LAGOPUS_RESULT_NO_MEMORY;
    } else {
      return LAGOPUS_RESULT_NOT_DEFINED;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
lagopus_session_tls_set_client_cert(const char *new_certfile) {
  if (IS_VALID_STRING(new_certfile) == true) {
    if (strlen(new_certfile) < PATH_MAX) {
      (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
      {
        snprintf(tls.session_cert, sizeof(tls.session_cert),
                 "%s", new_certfile);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
lagopus_session_tls_get_client_cert(char **certfile) {
  if (certfile != NULL && *certfile == NULL) {
    if (IS_VALID_STRING(tls.session_cert) == true) {
      char *str = NULL;

      (void)lagopus_rwlock_reader_lock(&(tls.s_lck));
      {
        str = strdup(tls.session_cert);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));

      if (str != NULL) {
        *certfile = str;
        return LAGOPUS_RESULT_OK;
      }
      free((void *)str);
      return LAGOPUS_RESULT_NO_MEMORY;
    } else {
      return LAGOPUS_RESULT_NOT_DEFINED;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
lagopus_session_tls_set_server_cert(const char *new_certfile) {
  if (IS_VALID_STRING(new_certfile) == true) {
    if (strlen(new_certfile) < PATH_MAX) {
      (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
      {
        snprintf(tls.session_cert, sizeof(tls.session_cert),
                 "%s", new_certfile);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
lagopus_session_tls_get_server_cert(char **certfile) {
  if (certfile != NULL && *certfile == NULL) {
    if (IS_VALID_STRING(tls.session_cert) == true) {
      char *str = NULL;

      (void)lagopus_rwlock_reader_lock(&(tls.s_lck));
      {
        str = strdup(tls.session_cert);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));

      if (str != NULL) {
        *certfile = str;
        return LAGOPUS_RESULT_OK;
      }
      free((void *)str);
      return LAGOPUS_RESULT_NO_MEMORY;
    } else {
      return LAGOPUS_RESULT_NOT_DEFINED;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
lagopus_session_tls_set_client_key(const char *new_pvtkey) {
  if (IS_VALID_STRING(new_pvtkey) == true) {
    if (strlen(new_pvtkey) < PATH_MAX) {
      (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
      {
        snprintf(tls.private_key, sizeof(tls.private_key),
                 "%s", new_pvtkey);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
lagopus_session_tls_get_client_key(char **pvtkey) {
  if (pvtkey != NULL && *pvtkey == NULL) {
    if (IS_VALID_STRING(tls.private_key) == true) {
      char *str = NULL;

      (void)lagopus_rwlock_reader_lock(&(tls.s_lck));
      {
        str = strdup(tls.private_key);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));

      if (str != NULL) {
        *pvtkey = str;
        return LAGOPUS_RESULT_OK;
      }
      free((void *)str);
      return LAGOPUS_RESULT_NO_MEMORY;
    } else {
      return LAGOPUS_RESULT_NOT_DEFINED;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
lagopus_session_tls_set_server_key(const char *new_pvtkey) {
  if (IS_VALID_STRING(new_pvtkey) == true) {
    if (strlen(new_pvtkey) < PATH_MAX) {
      (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
      {
        snprintf(tls.private_key, sizeof(tls.private_key),
                 "%s", new_pvtkey);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
lagopus_session_tls_get_server_key(char **pvtkey) {
  if (pvtkey != NULL && *pvtkey == NULL) {
    if (IS_VALID_STRING(tls.private_key) == true) {
      char *str = NULL;

      (void)lagopus_rwlock_reader_lock(&(tls.s_lck));
      {
        str = strdup(tls.private_key);
      }
      (void)lagopus_rwlock_unlock(&(tls.s_lck));

      if (str != NULL) {
        *pvtkey = str;
        return LAGOPUS_RESULT_OK;
      }
      free((void *)str);
      return LAGOPUS_RESULT_NO_MEMORY;
    } else {
      return LAGOPUS_RESULT_NOT_DEFINED;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
lagopus_session_tls_set_trust_point_conf(const char *new_tpconf) {
  if (IS_VALID_STRING(new_tpconf) == true) {
    if (strlen(new_tpconf) < PATH_MAX) {
      s_check_certificates_set_config_file(new_tpconf);
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
lagopus_session_tls_get_trust_point_conf(char **tpconf) {
  if (tpconf != NULL && *tpconf == NULL) {
    char *str = NULL;

    str = s_check_certificates_get_config_file();
    if (str != NULL) {
      *tpconf = str;
      return LAGOPUS_RESULT_OK;
    }
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

void
lagopus_session_tls_set_certcheck_default(lagopus_result_t
    (*func)(const char *, const char *)) {
  (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
  {
    check_certificates_default = func;
  }
  (void)lagopus_rwlock_unlock(&(tls.s_lck));
}

/* Assume locked. */
void
lagopus_session_tls_set_certcheck(lagopus_session_t s, lagopus_result_t
                                  (*func)(const char *, const char *)) {
  GET_TLS_CTX(s)->check_certificates = func;
}

static lagopus_result_t
stat_result_error(int error) {
  lagopus_result_t ret;
  if (error == EACCES) {
    ret = LAGOPUS_RESULT_NOT_ALLOWED;
  } else if (error == ENAMETOOLONG) {
    ret = LAGOPUS_RESULT_TOO_LONG;
  } else if (error == ENOENT || error == ENOTDIR) {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  } else if (error == ENOMEM) {
    ret = LAGOPUS_RESULT_NO_MEMORY;
  } else {
    ret = LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  return ret;
}

static lagopus_result_t
valid_path(const char *path, const bool is_file) {
  struct stat st;
  if (stat(path, &st) == 0) {
    if ((st.st_mode & S_IFMT) == S_IFDIR) {
      if (is_file == false) {
        return LAGOPUS_RESULT_OK;
      } else {
        return LAGOPUS_RESULT_INVALID_ARGS;
      }
    } else {
      if (is_file == true) {
        return LAGOPUS_RESULT_OK;
      } else {
        return LAGOPUS_RESULT_INVALID_ARGS;
      }
    }
  } else {
    return stat_result_error(errno);
  }
}

lagopus_result_t
session_tls_init(lagopus_session_t s) {

  if ((valid_path(tls.session_ca_dir, false) != LAGOPUS_RESULT_OK)
      ||  (valid_path(tls.session_cert, true) != LAGOPUS_RESULT_OK)
      ||  (valid_path(tls.private_key, true)  != LAGOPUS_RESULT_OK)) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  pthread_once(&initialized, initialize_internal);

  s->ctx = malloc(sizeof(struct tls_ctx));
  if (s->ctx == NULL) {
    lagopus_msg_warning("no memory.\n");
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  lagopus_mutex_lock(&lock);
  (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
  {
    GET_TLS_CTX(s)->ca_dir = strdup(tls.session_ca_dir);
  }
  (void)lagopus_rwlock_unlock(&(tls.s_lck));
  if (GET_TLS_CTX(s)->ca_dir == NULL) {
    lagopus_msg_warning("no memory.\n");
    free(s->ctx);
    lagopus_mutex_unlock(&lock);
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  if (s->session_type & SESSION_PASSIVE) {
    (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
    {
      GET_TLS_CTX(s)->cert = strdup(tls.session_cert);
    }
    (void)lagopus_rwlock_unlock(&(tls.s_lck));
    if (GET_TLS_CTX(s)->cert == NULL) {
      lagopus_msg_warning("no memory.\n");
      free(GET_TLS_CTX(s)->ca_dir);
      free(s->ctx);
      lagopus_mutex_unlock(&lock);
      return LAGOPUS_RESULT_NO_MEMORY;
    }
  } else if (s->session_type & SESSION_ACTIVE) {
    (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
    {
      GET_TLS_CTX(s)->cert = strdup(tls.session_cert);
    }
    (void)lagopus_rwlock_unlock(&(tls.s_lck));
    if (GET_TLS_CTX(s)->cert == NULL) {
      lagopus_msg_warning("no memory.\n");
      free(GET_TLS_CTX(s)->ca_dir);
      free(s->ctx);
      lagopus_mutex_unlock(&lock);
      return LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    GET_TLS_CTX(s)->key = NULL;
  }

  if (s->session_type & SESSION_PASSIVE) {
    (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
    {
      GET_TLS_CTX(s)->key = strdup(tls.private_key);
    }
    (void)lagopus_rwlock_unlock(&(tls.s_lck));
    if (GET_TLS_CTX(s)->key == NULL) {
      lagopus_msg_warning("no memory.\n");
      free(GET_TLS_CTX(s)->cert);
      free(GET_TLS_CTX(s)->ca_dir);
      free(s->ctx);
      lagopus_mutex_unlock(&lock);
      return LAGOPUS_RESULT_NO_MEMORY;
    }
  } else if (s->session_type & SESSION_ACTIVE) {
    (void)lagopus_rwlock_writer_lock(&(tls.s_lck));
    {
      GET_TLS_CTX(s)->key = strdup(tls.private_key);
    }
    (void)lagopus_rwlock_unlock(&(tls.s_lck));
    if (GET_TLS_CTX(s)->key == NULL) {
      lagopus_msg_warning("no memory.\n");
      free(GET_TLS_CTX(s)->cert);
      free(GET_TLS_CTX(s)->ca_dir);
      free(s->ctx);
      lagopus_mutex_unlock(&lock);
      return LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    GET_TLS_CTX(s)->key = NULL;
  }
  lagopus_mutex_unlock(&lock);

  if (s->session_type & SESSION_PASSIVE) {
    GET_TLS_CTX(s)->ctx =
      get_server_ssl_ctx(GET_TLS_CTX(s)->ca_dir,
                         GET_TLS_CTX(s)->cert, GET_TLS_CTX(s)->key);
    if (GET_TLS_CTX(s)->ctx == NULL) {
      free(GET_TLS_CTX(s)->key);
      free(GET_TLS_CTX(s)->cert);
      free(GET_TLS_CTX(s)->ca_dir);
      free(s->ctx);
      return LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    GET_TLS_CTX(s)->ctx = NULL;
  }

  GET_TLS_CTX(s)->ssl = NULL;
  GET_TLS_CTX(s)->verified = false;
  GET_TLS_CTX(s)->check_certificates = check_certificates_default;
  s->accept = accept_tls;
  s->connect = connect_tls;
  s->read = read_tls;
  s->write = write_tls;
  s->close = close_tls;
  s->destroy = destroy_tls;
  s->connect_check = connect_check_tls;

  return LAGOPUS_RESULT_OK;
}

static int
check_cert_chain(const lagopus_session_t s) {
  int ret = -1;
  char *issuer = NULL, *subject = NULL;
  X509 *peer = NULL;
  SSL *ssl = GET_TLS_CTX(s)->ssl;

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
  if (subject == NULL) {
    lagopus_msg_warning("no memory, get_subject_name failed.\n");
    ret = -1;
    goto done;
  }

  if (GET_TLS_CTX(s)->check_certificates != NULL) {
    if (GET_TLS_CTX(s)->check_certificates(issuer, subject) == LAGOPUS_RESULT_OK) {
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
connect_tls(lagopus_session_t s, const char *host, const char *port) {
  int ret;
  BIO *sbio;

  (void) host;
  (void) port;

  lagopus_msg_debug(10, "tls handshake start.\n");

  if (IS_TLS_NOT_INIT(s)) {
    SSL_CTX *ssl_ctx;

    ssl_ctx = get_client_ssl_ctx(GET_TLS_CTX(s)->ca_dir, GET_TLS_CTX(s)->cert,
                                 GET_TLS_CTX(s)->key);
    if (ssl_ctx == NULL) {
      lagopus_msg_warning("get_client_ssl_ctx() fail.\n");
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
                 && SSL_get_error(GET_TLS_CTX(s)->ssl, ret) != SSL_ERROR_WANT_WRITE)) {
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
    ret = check_cert_chain(s);
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
destroy_tls(lagopus_session_t s) {
  if (IS_TLS_NOT_INIT(s) == false) {
    free(GET_TLS_CTX(s)->ca_dir);
    free(GET_TLS_CTX(s)->cert);
    free(GET_TLS_CTX(s)->key);
    SSL_free(GET_TLS_CTX(s)->ssl);
    GET_TLS_CTX(s)->ssl = NULL;
    SSL_CTX_free(GET_TLS_CTX(s)->ctx);
    GET_TLS_CTX(s)->ctx = NULL;
  }

  lagopus_rwlock_destroy(&(tls.s_lck));

  free(GET_TLS_CTX(s));
  s->ctx =  NULL;
}

static void
close_tls(lagopus_session_t s) {
  if (IS_TLS_NOT_INIT(s) == false) {
    SSL_shutdown(GET_TLS_CTX(s)->ssl);
    SSL_clear(GET_TLS_CTX(s)->ssl);
  }
}

static lagopus_result_t
connect_check_tls(lagopus_session_t s) {
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





static inline void
s_init(void) {
  pthread_once(&initialized, initialize_internal);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(10, "The session/TLS module is initialized.\n");
}


static void
s_dtors(void) {
  if (lagopus_module_is_unloading() &&
      lagopus_module_is_finalized_cleanly()) {
    s_checkcert_final();

    lagopus_msg_debug(10, "The session/TLS module is finalized.\n");
  } else {
    lagopus_msg_debug(10, "The session/TLS module  is not finalized "
                      "because of module finalization problem.\n");
  }
}

