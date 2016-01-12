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

lagopus_result_t session_tcp_init(lagopus_session_t );

static ssize_t
read_tcp(lagopus_session_t s, void *buf, size_t n) {
  return read(s->sock, buf, n);
}

static ssize_t
write_tcp(lagopus_session_t s, void *buf, size_t n) {
  return write(s->sock, buf, n);
}

lagopus_result_t
session_tcp_init(lagopus_session_t s) {
  s->read = read_tcp;
  s->write = write_tcp;

  return LAGOPUS_RESULT_OK;
}
