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

extern struct session *session_tcp_init(struct session *);
extern struct session *session_tls_init(struct session *);
extern void *session_tls_destroy(struct session *);

/* Set socket buffer size to val. */
static void
socket_buffer_size_set(int sock, int optname, int val) {
  int ret;
  int size;
  socklen_t len = sizeof(size);

  /* Get current buffer size. */
  ret = getsockopt(sock, SOL_SOCKET, optname, &size, &len);
  if (ret < 0) {
    return;
  }

  /* If the buffer size is less than val, try to increase it. */
  if (size < val) {
    size = val;
    len = sizeof(size);
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, len);
    if (ret < 0) {
      return;
    }
  }

  /* Get buffer size. */
  ret = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, &len);
  if (ret < 0) {
    return;
  }

  /* Logging. */
  if (optname == SO_SNDBUF) {
    lagopus_msg_info("SO_SNDBUF buffer size is %d\n", size);
  } else {
    lagopus_msg_info("SO_RCVBUF buffer size is %d\n", size);
  }
}

/* TCP socket with non-blocking. */
static int
socket_create(int domain, int type, int protocol) {
  int sock;
  int val;

  /* Create socket. */
  sock = socket(domain, type, protocol);
  if (sock < 0) {
    lagopus_msg_warning("connect error %s\n", strerror(errno));
    return sock;
  }

  /* Non-blocking socket. */
  val = 1;
  ioctl(sock, FIONBIO, &val);

  /* Set socket send and receive buffer size to 64K. */
  socket_buffer_size_set(sock, SO_SNDBUF, 65535);
  socket_buffer_size_set(sock, SO_RCVBUF, 65535);

  return sock;
}

static int
bind_default(struct session *s, const char *host, const char *port) {
  int ret;
  int sock = -1;
  char service[NI_MAXSERV];
  struct addrinfo hints = {0,0,0,0,0,NULL,NULL,NULL}, *res = NULL;

  snprintf(service, sizeof(service), "%s", port);
  lagopus_msg_info("host:%s, service:%s\n", host, service);
  hints.ai_family = s->family;
  hints.ai_socktype = s->type;
  ret = getaddrinfo(host, service, &hints, &res);
  if (ret != 0) {
    lagopus_msg_warning("getaddrinfo error %s\n", gai_strerror(ret));
    freeaddrinfo(res);
    return -1;
  }

  sock = socket_create(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock < 0) {
    freeaddrinfo(res);
    return -1;
  }

  ret = bind(sock, res->ai_addr, res->ai_addrlen);
  if (ret < 0 && errno != EINPROGRESS) {
    lagopus_msg_warning("connect error %s\n", strerror(errno));
    freeaddrinfo(res);
    return ret;
  }

  s->sock = sock;
  freeaddrinfo(res);
  return ret;
}

static lagopus_result_t
connect_default(struct session *s, const char *host, const char *port) {
  int ret;
  int sock = -1;
  char service[NI_MAXSERV];
  struct addrinfo hints = {0,0,0,0,0,NULL,NULL,NULL}, *res = NULL;

  snprintf(service, sizeof(service), "%s", port);
  lagopus_msg_info("host:%s, service:%s\n", host, service);
  hints.ai_family = s->family;
  hints.ai_socktype = s->type;
  ret = getaddrinfo(host, service, &hints, &res);
  if (ret != 0) {
    lagopus_msg_warning("getaddrinfo error %s\n", gai_strerror(ret));
    freeaddrinfo(res);
    return -1;
  }

  if (s->sock < 0) {
    sock = socket_create(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
      freeaddrinfo(res);
      return -1;
    }
    s->sock = sock;
  }

  ret = connect(s->sock, res->ai_addr, res->ai_addrlen);
  if (ret < 0 && errno != EINPROGRESS) {
    lagopus_msg_warning("connect error %s\n", strerror(errno));
    close(s->sock);
    s->sock = -1;
    freeaddrinfo(res);
    return ret;
  }

  freeaddrinfo(res);
  return ret;
}

static void
close_default(struct session *s) {
  if (s->sock != -1) {
    (void)close(s->sock);
    s->sock = -1;
  }
}

struct session *
session_create(session_type_t t) {
  struct session *s = malloc(sizeof(struct session));

  if (s == NULL) {
    return NULL;
  }

  s->sock = -1;
  s->connect = connect_default;
  s->read = NULL;
  s->write = NULL;
  s->close = close_default;
  s->destroy = NULL;
  s->connect_check = NULL;
  s->ctx = NULL;
  s->session_type = t;

  switch (t) {
    case SESSION_TCP:
      s->family = PF_INET;
      s->type   = SOCK_STREAM;
      return session_tcp_init(s);
    case SESSION_TLS:
      s->family = PF_INET;
      s->type   = SOCK_STREAM;
      return session_tls_init(s);
    case SESSION_TCP6:
      s->family = PF_INET6;
      s->type   = SOCK_STREAM;
      return session_tcp_init(s);
    case SESSION_TLS6:
      s->family = PF_INET6;
      s->type   = SOCK_STREAM;
      return session_tls_init(s);
    default:
      free(s);
      return NULL;
  }
}

void
session_destroy(struct session *s) {
  if (s == NULL) {
    return;
  }
  close_default(s);
  s->connect = NULL;
  s->read = NULL;
  s->write = NULL;
  s->close = NULL;
  s->connect_check = NULL;

  if (s->destroy) {
    s->destroy(s);
  }

  s->destroy = NULL;
  free(s);
}

bool
session_is_alive(struct session *s) {
  if (s == NULL) {
    return false;
  } else if (s->sock == -1) {
    return false;
  }

  return true;
}

lagopus_result_t
session_connect(struct session *s, struct addrunion *daddr, uint16_t dport,
                struct addrunion *saddr, uint16_t sport) {
  char addr[256];
  char port[16];
  lagopus_result_t ret;

  if (saddr != NULL) {
    inet_ntop(saddr->family, &saddr->addr4, addr, sizeof(addr));
    snprintf(port, sizeof(port), "%d", sport);
    ret = bind_default(s, addr, port);
    if (ret < 0) {
      lagopus_msg_warning("bind_default error\n");
      return LAGOPUS_RESULT_TLS_CONN_ERROR;
    }
  }

  inet_ntop(daddr->family, &daddr->addr4, addr, sizeof(addr));
  snprintf(port, sizeof(port), "%d", dport);

  ret = connect_default(s, addr, port);
  if (ret < 0) {
    return ret;
  }

  if (s->connect != NULL) {
    ret = s->connect(s, addr, port);
  }

  return ret;
}

lagopus_result_t
session_connect_check(struct session *s) {
  int ret0, value;
  lagopus_result_t ret1;
  socklen_t len = sizeof(value);

  if (s == NULL) {
    lagopus_msg_warning("session is NULL\n");
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  ret0 = getsockopt(s->sock, SOL_SOCKET, SO_ERROR, (void *)&value, &len);
  if (ret0 < 0) {
    lagopus_msg_warning("getsockopt error %d\n", errno);
    return LAGOPUS_RESULT_ANY_FAILURES;
  } else if (value != 0) {
    lagopus_msg_warning("getsockopt value error %d\n", value);
    return LAGOPUS_RESULT_SOCKET_ERROR;
  }

  if (s->connect_check) {
    ret1 = s->connect_check(s);
  } else {
    ret1 = LAGOPUS_RESULT_OK;
  }

  return ret1;
}

void
session_close(struct session *s) {
  if (s->close) {
    s->close(s);
  } else {
    close_default(s);
  }
}

ssize_t
session_read(struct session *s, void *buf, size_t n) {
  if (s == NULL || s->read == NULL || buf == NULL) {
    lagopus_msg_warning("session_read: invalid args\n");
    return -1;
  }

  return s->read(s, buf, n);
}

ssize_t
session_write(struct session *s, void *buf, size_t n) {
  if (s == NULL || s->write == NULL || buf == NULL) {
    lagopus_msg_warning("session_read: invalid args\n");
    return -1;
  }

  return s->write(s, buf, n);
}

int
session_sockfd_get(struct session *s) {
  return s->sock;
}

void
session_sockfd_set(struct session *s, int sock) {
  if (s->sock >= 0) {
    close(s->sock);
  }
  s->sock = sock;
}

void
session_write_set(struct session *s, ssize_t (*writep)(struct session *,
                  void *, size_t)) {
  s->write = writep;
}
