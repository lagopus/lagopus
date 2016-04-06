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
#include "session_internal.h"

#define SBUF_UNREAD_LEN(a) ((((a)->rbuf.ep) - ((a)->rbuf.rp)))
#define SBUF_UNFILL_LEN(a) (((a)->rbuf.buf + SESSION_BUFSIZ) - ((a)->rbuf.ep))

#define MAX_EVENTS     1024

extern lagopus_result_t session_tcp_init(lagopus_session_t );
extern lagopus_result_t session_tls_init(lagopus_session_t );

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
    lagopus_msg_debug(10, "SO_SNDBUF buffer size is %d\n", size);
  } else {
    lagopus_msg_debug(10, "SO_RCVBUF buffer size is %d\n", size);
  }
}

/* TCP socket with non-blocking. */
static int
socket_create(int domain, int type, int protocol, bool nonblocking) {
  int sock;
  int val;

  /* Create socket. */
  sock = socket(domain, type, protocol);
  if (sock < 0) {
    lagopus_msg_warning("connect error %s.\n", strerror(errno));
    return sock;
  }

  if (nonblocking) {
    /* Non-blocking socket. */
    val = 1;
    ioctl(sock, FIONBIO, &val);
  }

  /* Set socket send and receive buffer size to 64K. */
  socket_buffer_size_set(sock, SO_SNDBUF, 65535);
  socket_buffer_size_set(sock, SO_RCVBUF, 65535);

  return sock;
}

static lagopus_result_t
sockaddr_get(const lagopus_ip_address_t *ip,
             uint16_t port,
             struct sockaddr **saddr,
             socklen_t *saddr_len) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_ipv4 = false;

  if (ip != NULL && saddr != NULL &&
      *saddr == NULL && saddr_len != NULL) {
    if ((ret = lagopus_ip_address_sockaddr_get(ip, saddr)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = lagopus_ip_address_sockaddr_len_get(ip, saddr_len)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = lagopus_ip_address_is_ipv4(ip, &is_ipv4)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if (is_ipv4 == true) {
      ((struct sockaddr_in *) *saddr)->sin_port = htons(port);
    } else {
      ((struct sockaddr_in6 *) *saddr)->sin6_port = htons(port);
    }

    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static int
bind_default(lagopus_session_t s,
             const lagopus_ip_address_t *addr,
             uint16_t port) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  int ret;
  int sock = -1;
  socklen_t saddr_len = 0;
  struct sockaddr *saddr = NULL;

  if ((r = sockaddr_get(addr, port, &saddr, &saddr_len)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    free(saddr);
    return -1;
  }

  if (s->family != saddr->sa_family) {
    lagopus_msg_error("Bad address family (%"PRIu8" != %"PRIu8")\n",
                      s->family, saddr->sa_family);
    free(saddr);
    return -1;
  }

  sock = socket_create(s->family, s->type, s->protocol,
                       s->session_type & SESSION_ACTIVE ? true : false);
  if (sock < 0) {
    free(saddr);
    return -1;
  }

  if (s->session_type & SESSION_PASSIVE) {
    const int on = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret < 0) {
      lagopus_msg_error("setsockopt error %s.\n", strerror(errno));
      free(saddr);
      close(sock);
      return ret;
    }
  }

  ret = bind(sock, saddr, saddr_len);
  if (ret < 0 && errno != EINPROGRESS) {
    lagopus_msg_error("connect error %s.\n", strerror(errno));
    free(saddr);
    close(sock);
    return ret;
  }

  s->sock = sock;
  free(saddr);

  return ret;
}

static lagopus_result_t
connect_default(lagopus_session_t s,
                const lagopus_ip_address_t *addr,
                uint16_t port) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  int ret;
  int sock = -1;
  socklen_t saddr_len = 0;
  struct sockaddr *saddr = NULL;

  if ((r = sockaddr_get(addr, port, &saddr, &saddr_len)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    free(saddr);
    return -1;
  }

  if (s->family != saddr->sa_family) {
    lagopus_msg_error("Bad address family ( %"PRIu8").\n",
                      saddr->sa_family);
    free(saddr);
    return -1;
  }

  if (s->sock < 0) {
    sock = socket_create(s->family, s->type, s->protocol,
                         s->session_type & SESSION_ACTIVE ? true : false);
    if (sock < 0) {
      free(saddr);
      return -1;
    }
    s->sock = sock;
  }

  ret = connect(s->sock, saddr, saddr_len);
  if (ret < 0 && errno != EINPROGRESS) {
    lagopus_msg_error("connect error %s\n", strerror(errno));
    close(s->sock);
    s->sock = -1;
    free(saddr);
    return ret;
  }

  free(saddr);
  return ret;
}

static void
close_default(lagopus_session_t s) {
  if (s->sock != -1) {
    (void)close(s->sock);
    s->sock = -1;
  }
}

static ssize_t
writen(lagopus_session_t s, void *buf, size_t n) {
  ssize_t ret, offset = 0;

  while (n > 0) {
    if (s->write != NULL) {
      ret = s->write(s, buf + offset, n);
      if (ret < 0 && offset == 0) {
        return ret;
      } else if (ret <= 0) {
        return offset;
      } else {
        offset += ret;
        n -= (size_t) ret;
      }
    } else {
      return LAGOPUS_RESULT_INVALID_OBJECT;
    }
  }

  return offset;
}

lagopus_result_t
session_create(session_type_t t, lagopus_session_t *session) {
  lagopus_result_t ret;
  lagopus_session_t s;

  if (!(t & (SESSION_PASSIVE | SESSION_ACTIVE |
            SESSION_ACCEPTED | SESSION_UNIX_STREAM | SESSION_UNIX_DGRAM))) {
    lagopus_msg_warning("illegal session type: 0x%x\n", t);
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (session == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  lagopus_msg_debug(5, "s_server_session %x.\n", t);

  s = malloc(sizeof(struct session));
  if (s == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  s->sock = -1;
  s->accept = NULL;
  s->connect = NULL;
  s->read = NULL;
  s->write = NULL;
  s->close = close_default;
  s->destroy = NULL;
  s->connect_check = NULL;
  s->ctx = NULL;
  s->session_type = t;
  s->events = 0;
  memset(&s->rbuf.buf, 0, sizeof(s->rbuf.buf));
  s->rbuf.rp = s->rbuf.ep = s->rbuf.buf;

  lagopus_msg_debug(5, "s_server_session %x, SESSION_TCP %x.\n", t, SESSION_TCP);
  if (t & SESSION_TCP) {
    s->family = PF_INET;
    s->type   = SOCK_STREAM;
    s->protocol = IPPROTO_TCP;
  } else if (t & SESSION_TCP6) {
    s->family = PF_INET6;
    s->type   = SOCK_STREAM;
    s->protocol = IPPROTO_TCP;
  } else if (t & SESSION_UNIX_STREAM) {
    s->family = PF_UNIX;
    s->type   = SOCK_STREAM;
    s->protocol = 0;
  } else if (t & SESSION_UNIX_DGRAM) {
    s->family = PF_UNIX;
    s->type   = SOCK_DGRAM;
    s->protocol = 0;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    goto err;
  }

  lagopus_msg_debug(5, "s_server_session %x, SESSION_TLS %x.\n", t, SESSION_TLS);
  lagopus_msg_debug(5, "lagopus_session_t s %p.\n", s);
  if (t & SESSION_TLS) {
    lagopus_msg_debug(5, "session_tls_init.\n");
    ret = session_tls_init(s);
    if (ret != LAGOPUS_RESULT_OK) {
      goto err;
    }
  } else if (t & (SESSION_TCP | SESSION_TCP6 | SESSION_UNIX_STREAM | SESSION_UNIX_DGRAM)) {
    ret = session_tcp_init(s);
    if (ret != LAGOPUS_RESULT_OK) {
      goto err;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    goto err;
  }

  *session = s;
  return LAGOPUS_RESULT_OK;

err:
  lagopus_msg_warning("illegal session type: 0x%x\n", t);
  free(s);
  *session = NULL;
  return ret;
}

lagopus_result_t
session_pair(session_type_t t, lagopus_session_t session[2]) {
  int ret0, sock[2];
  lagopus_result_t ret;

  if (!(t & (SESSION_UNIX_STREAM|SESSION_UNIX_DGRAM))) {
    lagopus_msg_warning("illegal session type: 0x%x\n", t);
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  ret = session_create(t, &session[0]);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    return ret;
  }

  ret = session_create(t, &session[1]);
  if (ret != LAGOPUS_RESULT_OK) {
    session_destroy(session[0]);
    lagopus_perror(ret);
    return ret;
  }

  ret0 = socketpair(session[0]->family, session[0]->type, 0, sock);
  if (ret0 != 0) {
    ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    session_destroy(session[0]);
    session_destroy(session[1]);
    return ret;
  }

  session[0]->sock = sock[0];
  session[1]->sock = sock[1];

  return LAGOPUS_RESULT_OK;
}

void
session_destroy(lagopus_session_t s) {
  if (s == NULL) {
    return;
  }
  close_default(s);
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
session_is_alive(lagopus_session_t s) {
  if (s == NULL) {
    return false;
  } else if (s->sock == -1) {
    return false;
  }

  return true;
}

void
session_event_clear(lagopus_session_t s) {
  s->events = 0;
  s->revents = 0;
}

void
session_read_event_set(lagopus_session_t s) {
  s->events |= POLLIN;
  s->revents = 0;
}

void
session_read_event_unset(lagopus_session_t s) {
  s->events = s->events & ~POLLIN;
  s->revents = 0;
}

lagopus_result_t
session_is_readable(lagopus_session_t s, bool *b) {
  if (s->revents & POLLIN) {
    *b = true;
    return LAGOPUS_RESULT_OK;
  } else if (s->revents & (POLLERR|POLLHUP|POLLNVAL)) {
    *b = false;
    return LAGOPUS_RESULT_SOCKET_ERROR;
  } else {
    *b = false;
    return LAGOPUS_RESULT_OK;
  }
}

lagopus_result_t
session_is_read_on(lagopus_session_t s, bool *b) {
  if (s->events & POLLIN) {
    *b = true;
    return LAGOPUS_RESULT_OK;
  } else {
    *b = false;
    return LAGOPUS_RESULT_OK;
  }
}

void
session_write_event_set(lagopus_session_t s) {
  s->events |= POLLOUT;
  s->revents = 0;
}

void
session_write_event_unset(lagopus_session_t s) {
  s->events = s->events & ~POLLOUT;
  s->revents = 0;
}

lagopus_result_t
session_is_writable(lagopus_session_t s, bool *b) {
  if (s->revents & POLLOUT) {
    *b = true;
    return LAGOPUS_RESULT_OK;
  } else if (s->revents & (POLLERR|POLLHUP|POLLNVAL)) {
    *b = false;
    return LAGOPUS_RESULT_SOCKET_ERROR;
  } else {
    *b = false;
    return LAGOPUS_RESULT_OK;
  }
}

lagopus_result_t
session_is_write_on(lagopus_session_t s, bool *b) {
  if (s->events & POLLOUT) {
    *b = true;
    return LAGOPUS_RESULT_OK;
  } else {
    *b = false;
    return LAGOPUS_RESULT_OK;
  }
}

bool
session_is_passive(lagopus_session_t s) {
  return (s->session_type & SESSION_PASSIVE);
}

lagopus_result_t
session_poll(lagopus_session_t s[], int n, int timeout) {
  int i, n_events = 0;
  lagopus_result_t ret;
  struct pollfd pollfd[MAX_EVENTS];

  if (n > MAX_EVENTS) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = 0; i < n; i++) {
    if ((s[i]->events & POLLIN) && (SBUF_UNREAD_LEN(s[i]) > 0)) {
      s[i]->revents = POLLIN;
      n_events++;
    } else {
      pollfd[i].fd = s[i]->sock;
      pollfd[i].events = s[i]->events;
      pollfd[i].revents = 0;
    }
  }

  if (n_events > 0) {
    return n_events;
  }

  lagopus_msg_debug(10, "%d events pollin.\n", n);
  n_events = poll(pollfd, (nfds_t) n, timeout);
  lagopus_msg_debug(10, "%d events pollout.\n", n_events);
  if (n_events == 0) {
    ret = LAGOPUS_RESULT_TIMEDOUT;
  } else if (n_events < 0) {
    if (errno == EINTR) {
      ret = LAGOPUS_RESULT_INTERRUPTED;
    } else {
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = n_events;
  }

  for (i = 0; i < n; i++) {
    s[i]->revents = pollfd[i].revents;
  }

  return ret;
}

lagopus_result_t
session_accept(lagopus_session_t s1, lagopus_session_t *s2) {
  int sock;
  struct sockaddr_storage ss = {0,0,{0}};
  socklen_t ss_len = sizeof(ss);
  session_type_t t;
  lagopus_result_t ret;

  sock = accept(s1->sock, (struct sockaddr *) &ss, &ss_len);
  if (sock  < 0) {
    lagopus_msg_warning("accept error.\n");
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }

  t = ((s1->session_type
        & (unsigned int) ~(SESSION_PASSIVE|SESSION_ACTIVE)) | SESSION_ACCEPTED);
  ret = session_create(t, s2);
  if (*s2 == NULL) {
    close(sock);
    return ret;
  }
  (*s2)->sock = sock;

  if (s1->accept != NULL) {
    return s1->accept(s1, s2);
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
session_bind(lagopus_session_t s, lagopus_ip_address_t *saddr, uint16_t sport) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  int ret;

  if (s == NULL || saddr == NULL || sport == 0) {
    lagopus_msg_warning("session is null.\n");
    r = LAGOPUS_RESULT_ANY_FAILURES;
    goto done;
  }

  ret = bind_default(s, saddr, sport);
  if (ret < 0) {
    lagopus_msg_warning("bind_default error.\n");
    r = LAGOPUS_RESULT_POSIX_API_ERROR;
    goto done;
  }

  if (s->session_type & SESSION_PASSIVE) {
    ret = listen(s->sock, SOMAXCONN);
    if (ret < 0) {
      r = LAGOPUS_RESULT_POSIX_API_ERROR;
      goto done;
    }
  }

  r = LAGOPUS_RESULT_OK;

done:
  return r;
}

lagopus_result_t
session_connect(lagopus_session_t s, lagopus_ip_address_t *daddr, uint16_t dport,
                lagopus_ip_address_t *saddr, uint16_t sport) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *d_addr = NULL;
  char port[16];

  if (s == NULL) {
    lagopus_msg_warning("session is null.\n");
    ret = LAGOPUS_RESULT_ANY_FAILURES;
    goto done;
  }

  if (!(s->session_type & SESSION_ACTIVE)) {
    lagopus_msg_warning("not using passive socket.\n");
    ret = LAGOPUS_RESULT_TLS_CONN_ERROR;
    goto done;
  }

  if (saddr != NULL) {
    ret = bind_default(s, saddr, sport);
    if (ret < 0) {
      lagopus_msg_warning("bind_default error.\n");
      ret = LAGOPUS_RESULT_TLS_CONN_ERROR;
      goto done;
    }
  }

  ret = connect_default(s, daddr, dport);
  if (ret < 0) {
    goto done;
  }

  if (s->connect != NULL) {
    ret = s->connect(s, d_addr, port);
  }

done:

  return ret;
}

lagopus_result_t
session_connect_check(lagopus_session_t s) {
  int ret0, value;
  lagopus_result_t ret1;
  socklen_t len = sizeof(value);

  if (s == NULL) {
    lagopus_msg_warning("session is null.\n");
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  ret0 = getsockopt(s->sock, SOL_SOCKET, SO_ERROR, (void *)&value, &len);
  if (ret0 < 0) {
    lagopus_msg_warning("getsockopt error %d.\n", errno);
    return LAGOPUS_RESULT_ANY_FAILURES;
  } else if (value != 0) {
    lagopus_msg_warning("getsockopt value error %d.\n", value);
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
session_close(lagopus_session_t s) {
  if (s->close) {
    s->close(s);
  } else {
    close_default(s);
  }
}

ssize_t
session_read(lagopus_session_t s, void *buf, size_t n) {
  if (s == NULL || s->read == NULL || buf == NULL) {
    lagopus_msg_warning("session_read: invalid args.\n");
    return -1;
  }

  return s->read(s, buf, n);
}

ssize_t
session_write(lagopus_session_t s, void *buf, size_t n) {
  if (s == NULL || s->write == NULL || buf == NULL) {
    lagopus_msg_warning("session_write: invalid args.\n");
    return -1;
  }

  return s->write(s, buf, n);
}

int
session_sockfd_get(lagopus_session_t s) {
  return s->sock;
}

void
session_sockfd_set(lagopus_session_t s, int sock) {
  if (s->sock >= 0) {
    close(s->sock);
  }
  s->sock = sock;
}

void
session_write_set(lagopus_session_t s, ssize_t (*writep)(lagopus_session_t ,
                  void *, size_t)) {
  s->write = writep;
}

char *
session_fgets(char *restrict str, int size, lagopus_session_t s) {
  char   *c, *d;
  ssize_t len;

  if (size <= 0) {
    return NULL;
  }

  d = str;
  size--;
  while (size != 0) {
    len = SBUF_UNREAD_LEN(s);
    if (len <= 0) {
      len = s->read(s, s->rbuf.buf, SESSION_BUFSIZ);
      if (len <= 0) {
        return NULL;
      }
      s->rbuf.rp = s->rbuf.ep = s->rbuf.buf;
      s->rbuf.ep += len;
    }

    if (len > size) {
      len = (size_t) size;
    }

    c = memchr(s->rbuf.rp, '\n', (size_t) len);
    if (c != NULL) {
      len = (ssize_t) (c + 1 - s->rbuf.rp);
      memcpy(d, s->rbuf.rp, (size_t) len);
      d += len;
      s->rbuf.rp += len;
      break;
    }

    memcpy(d, s->rbuf.rp, (size_t) len);
    d += len;
    size -= (int) len;
    s->rbuf.rp += len;
  }

  *d = '\0';

  return str;
}

int
session_vprintf(lagopus_session_t s, const char *fmt, va_list ap) {
  int size;
  ssize_t ret;
  char *buf;

  size = vasprintf(&buf, fmt, ap);
  if (size < 0) {
    return size;
  }

  ret = writen(s, buf, (size_t) size);
  free(buf);

  return (int) ret;
}

int
session_printf(lagopus_session_t s, const char *fmt, ...) {
  int ret;
  va_list arg;

  va_start(arg, fmt);
  ret = session_vprintf(s, fmt, arg);
  va_end(arg);


  return ret;
}

bool
session_type_is_passive(const session_type_t session_type) {
  return session_type & SESSION_PASSIVE;
}

bool
session_type_is_active(const session_type_t session_type) {
  return session_type & SESSION_ACTIVE;
}

bool
session_type_is_accepted(const session_type_t session_type) {
  return session_type & SESSION_ACCEPTED;
}

bool
session_type_is_tcp(const session_type_t session_type) {
  return session_type & SESSION_TCP;
}

bool
session_type_is_tcp6(const session_type_t session_type) {
  return session_type & SESSION_TCP6;
}

bool
session_type_is_tls(const session_type_t session_type) {
  return session_type & SESSION_TLS;
}

uint64_t
session_id_get(lagopus_session_t s) {
  return (uint64_t) s->sock;
}
