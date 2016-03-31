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

#ifndef __SESSION_INTERNAL_H__
#define __SESSION_INTERNAL_H__

#define SESSION_BUFSIZ 4096

struct session {
  /* socket descriptor */
  int sock;
  int family;
  int type;
  int protocol;
  session_type_t session_type;
  short events; /* for session_event_poll */
  short revents; /* for session_event_poll */
  struct session_buf {
    char *rp;
    char *ep;
    char buf[SESSION_BUFSIZ];
  } rbuf;
  /* function pointers */
  lagopus_result_t (*connect)(lagopus_session_t s, const char *host,
                              const char *port);
  lagopus_result_t (*accept)(lagopus_session_t s1, lagopus_session_t *s2);
  ssize_t (*read)(lagopus_session_t, void *, size_t);
  ssize_t (*write)(lagopus_session_t, void *, size_t);
  void (*close)(lagopus_session_t);
  void (*destroy)(lagopus_session_t);
  lagopus_result_t (*connect_check)(lagopus_session_t);
  /* protocol depended context object */
  void *ctx;
};

#endif /* __SESSION_INTERNAL_H__ */

