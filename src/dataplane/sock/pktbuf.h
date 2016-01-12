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

#ifndef SRC_DATAPLANE_SOCK_PKTBUF_H_
#define SRC_DATAPLANE_SOCK_PKTBUF_H_

/**
 *      @file   pktbuf.h
 */

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#else
#include <net/if.h>
#include <net/if_ether.h>
#endif /* HAVE_NET_ETHERNET_H */
#ifdef __linux__
#include <linux/if_vlan.h>
#endif /* __linux__ */
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

struct sock_buf {
  size_t len;
  unsigned char *data;
  int refcnt;
  unsigned char dat[MAX_PACKET_SZ + 128];
};

#define OS_MBUF           struct sock_buf
#define OS_M_PKTLEN(m)    ((m)->len)
#define OS_M_PREPEND(m,n) ((m)->data -= (n), (m)->len += (n), (m)->data)
#define OS_M_APPEND(m,n)  ((m)->len += (n), (m)->data + (m)->len - (n))
#define OS_M_ADJ(m,n)     ((m)->data += (n), (m)->len -= (n))
#define OS_M_TRIM(m,n)    ((m)->len -= (n))
#define OS_M_FREE(m)      sock_m_free(m)
#define OS_MTOD(m,type)   ((type)(m)->data)
#define OS_M_ADDREF(m)    ((m)->refcnt++)
#define OS_NTOHS ntohs
#define OS_NTOHL ntohl
#ifdef LAGOPUS_BIG_ENDIAN
#define OS_NTOHLL(x) (x)
#else /* Little endian */
#define OS_NTOHLL(x) ((uint64_t)ntohl((x) & 0xffffffffLLU) << 32 \
                      | ntohl((x) >> 32 & 0xffffffffLLU))
#endif
#define OS_HTONS htons
#define OS_HTONL htonl
#ifdef LAGOPUS_BIG_ENDIAN
#define OS_HTONLL(x) (x)
#else /* Little endian */
#define OS_HTONLL(x) ((uint64_t)htonl((x) & 0xffffffffLLU) << 32 \
                      | htonl((x) >> 32 & 0xffffffffLLU))
#endif
#define OS_MEMCPY memcpy

#ifndef likely
#define likely(n) n
#endif
#ifndef unlikely
#define unlikely(n) n
#endif

void sock_m_free(OS_MBUF *);

#endif /* SRC_DATAPLANE_SOCK_PKTBUF_H_ */
