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

#include "lagopus/addrunion.h"

int
addrunion_ipv4_set(struct addrunion *addrunion, const char *str) {
  addrunion->family = AF_INET;
  return inet_pton(AF_INET, str, &addrunion->addr4);
}

int
addrunion_ipv6_set(struct addrunion *addrunion, const char *str) {
  addrunion->family = AF_INET6;
  return inet_pton(AF_INET6, str, &addrunion->addr6);
}

const char *
addrunion_ipaddr_str_get(struct addrunion *addrunion, char *dst,
                         socklen_t size) {
  return inet_ntop(addrunion->family, &addrunion->addr4, dst, size);
}

int
addrunion_af_get(struct addrunion *addrunion) {
  return addrunion->family;
}
