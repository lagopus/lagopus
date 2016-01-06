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

#ifndef __LAGOPUS_ADDRINFO_H__
#define __LAGOPUS_ADDRINFO_H__

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct addrunion {
  /* AF_INET or AF_INET6. */
  int family;

  union {
    struct in_addr addr4;
    struct in6_addr addr6;
  };
};

int
addrunion_ipv4_set(struct addrunion *addrunion, const char *str);

int
addrunion_ipv6_set(struct addrunion *addrunion, const char *str);

const char *
addrunion_ipaddr_str_get(struct addrunion *addrunion, char *dst,
                         socklen_t size);

int
addrunion_af_get(struct addrunion *addrunion);

#endif /* __LAGOPUS_ADDRINFO_H__ */
