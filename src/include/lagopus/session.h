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


#ifndef __SESSION_H__
#define __SESSION_H__
struct session;

typedef enum {
  SESSION_TCP   = 0,
  SESSION_TLS   = 1,
  SESSION_TCP6  = 2,
  SESSION_TLS6  = 3,
} session_type_t;


struct session *session_create(session_type_t);
void session_destroy(struct session *);
bool session_is_alive(struct session *);
lagopus_result_t
session_connect(struct session *s, struct addrunion *daddr, uint16_t dport,
                struct addrunion *saddr, uint16_t sport);
void session_close(struct session *s);
lagopus_result_t session_connect_check(struct session *s);

ssize_t
session_read(struct session *s, void *buf, size_t n);

ssize_t
session_write(struct session *s, void *buf, size_t n);

int
session_sockfd_get(struct session *s);

void
session_sockfd_set(struct session *s, int sock);

void
session_write_set(struct session *s, ssize_t (*write)(struct session *,
                  void *, size_t));

#endif /* __SESSION_H__ */
