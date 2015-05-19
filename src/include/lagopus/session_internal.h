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


#ifndef __SESSION_INTERNAL_H__
#define __SESSION_INTERNAL_H__

struct session {
  /* socket descriptor */
  int sock;
  int family;
  int type;
  session_type_t session_type;
  /* function pointers */
  lagopus_result_t (*connect)(struct session *s, const char *host,
                              const char *port);
  ssize_t (*read)(struct session *, void *, size_t);
  ssize_t (*write)(struct session *, void *, size_t);
  void (*close)(struct session *);
  void (*destroy)(struct session *);
  lagopus_result_t (*connect_check)(struct session *);
  /* protocol depended context object */
  void *ctx;
};
#endif /* __SESSION_INTERNAL_H__ */

