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

/**
 *      @file   csum.h
 *      @brief  IP/TCP/UDP Checksum calcuration for dataplane
 */

/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   Copyright 2014 6WIND S.A.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef SRC_DATAPLANE_OFPROTO_CSUM_H_
#define SRC_DATAPLANE_OFPROTO_CSUM_H_

#include <inttypes.h>
#include <netinet/ip_icmp.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"

#include "crc32.h"

#define HW_IP_CKSUM_IS_ENABLED 0
#define HW_TCP_CKSUM_IS_ENABLED 0
#define HW_UDP_CKSUM_IS_ENABLED 0
#define HW_SCTP_CKSUM_IS_ENABLED 0

static inline uint16_t
get_16b_sum(uint16_t *u16, uint32_t len, uint32_t sum) {
  while (len >= sizeof(uint16_t) * 4) {
    sum += u16[0];
    sum += u16[1];
    sum += u16[2];
    sum += u16[3];
    len -= (uint32_t)(sizeof(uint16_t) * 4);
    u16 += 4;
  }
  while (len > 1) {
    sum += *u16++;
    len -= (uint32_t)sizeof(uint16_t);
  }

  /* If length is in odd bytes */
  if (len == 1) {
    sum += *((const uint8_t *)u16);
  }

  sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
  sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
  return (uint16_t)sum;
}

static inline uint16_t
get_ipv4_cksum(uint16_t *ipv4_hdr) {
  uint16_t cksum;
  cksum = get_16b_sum(ipv4_hdr, sizeof(IPV4_HDR), 0);
  return (uint16_t)((cksum == 0xffff) ? cksum : ~cksum);
}

static inline uint16_t
get_ipv4_psd_sum(IPV4_HDR *ip_hdr) {
  /* Pseudo Header for IPv4/UDP/TCP checksum */
  union ipv4_psd_header {
    struct {
      uint32_t src_addr; /* IP address of source host. */
      uint32_t dst_addr; /* IP address of destination host(s). */
      uint8_t  zero;     /* zero. */
      uint8_t  proto;    /* L4 protocol type. */
      uint16_t len;      /* L4 length. */
    } __attribute__((__packed__));
    uint16_t u16_arr[0];
  } psd_hdr;

  psd_hdr.src_addr = IPV4_SRC(ip_hdr);
  psd_hdr.dst_addr = IPV4_DST(ip_hdr);
  psd_hdr.zero     = 0;
  psd_hdr.proto    = IPV4_PROTO(ip_hdr);
  psd_hdr.len      = OS_HTONS((uint16_t)(IPV4_TLEN(ip_hdr)
                                         - (IPV4_HLEN(ip_hdr) << 2)));
  return get_16b_sum(psd_hdr.u16_arr, sizeof(psd_hdr), 0);
}

static inline uint16_t
get_ipv6_psd_sum(IPV6_HDR *ip_hdr) {
  /* Pseudo Header for IPv6/UDP/TCP checksum */
  union ipv6_psd_header {
    struct {
      uint8_t src_addr[16]; /* IP address of source host. */
      uint8_t dst_addr[16]; /* IP address of destination host(s). */
      uint32_t len;         /* L4 length. */
      uint32_t proto;       /* L4 protocol - top 3 bytes must be zero */
    } __attribute__((__packed__));

    uint16_t u16_arr[0]; /* allow use as 16-bit values with safe aliasing */
  } psd_hdr;

  OS_MEMCPY(&psd_hdr.src_addr, IPV6_SRC(ip_hdr),
            sizeof(struct in6_addr) * 2);
  psd_hdr.len       = OS_HTONL(IPV6_PLEN(ip_hdr));
  psd_hdr.proto     = OS_HTONL((uint32_t)IPV6_PROTO(ip_hdr));

  return get_16b_sum(psd_hdr.u16_arr, sizeof(psd_hdr), 0);
}

static inline uint16_t
get_ipv4_l4_checksum(IPV4_HDR *ipv4_hdr, uint16_t *l4_hdr, uint32_t cksum) {
  uint32_t l4_len;

  l4_len = (uint32_t)(IPV4_TLEN(ipv4_hdr) - (IPV4_HLEN(ipv4_hdr) << 2));

  cksum += get_16b_sum(l4_hdr, l4_len, 0);

  cksum = ((cksum & 0xffff0000) >> 16) + (cksum & 0xffff);
  cksum = (~cksum) & 0xffff;
  if (cksum == 0) {
    cksum = 0xffff;
  }
  return (uint16_t)cksum;
}

static inline uint16_t
get_ipv6_l4_checksum(IPV6_HDR *ipv6_hdr, uint16_t *l4_hdr, uint32_t cksum) {
  uint32_t l4_len;

  l4_len = IPV6_PLEN(ipv6_hdr);

  cksum = get_16b_sum(l4_hdr, l4_len, cksum);

  cksum = (~cksum) & 0xffff;
  if (cksum == 0) {
    cksum = 0xffff;
  }

  return (uint16_t)cksum;
}

/**
 * Update IPv4 header checksum.
 *
 * @param[in]   pkt     packet for updating.
 */
static inline void
lagopus_update_iphdr_checksum(struct lagopus_packet *pkt) {
  if (HW_IP_CKSUM_IS_ENABLED) {
    /* calculate ip checksum by NIC, nothing to do. */
  } else {
    IPV4_CSUM(pkt->ipv4) = 0;
    IPV4_CSUM(pkt->ipv4) = get_ipv4_cksum(pkt->l3_hdr_w);
  }
}

/**
 * Update TCP checksum.
 *
 * @param[in]   pkt     packet for updating.
 */
static inline void
lagopus_update_tcp_checksum(struct lagopus_packet *pkt) {
  if (pkt->ether_type == ETHERTYPE_IP) {
    if (HW_TCP_CKSUM_IS_ENABLED) {
      /* calculate tcp checksum by NIC, nothing to do. */
      TCP_CKSUM(pkt->tcp) = get_ipv4_psd_sum(pkt->ipv4);
    } else {
      TCP_CKSUM(pkt->tcp) = 0;
      TCP_CKSUM(pkt->tcp) = get_ipv4_l4_checksum(pkt->ipv4, pkt->l4_hdr_w,
                            get_ipv4_psd_sum(pkt->ipv4));
    }
  } else if (pkt->ether_type == ETHERTYPE_IPV6) {
    if (HW_TCP_CKSUM_IS_ENABLED) {
      /* calculate tcp checksum by NIC, nothing to do. */
      TCP_CKSUM(pkt->tcp) = get_ipv6_psd_sum(pkt->ipv6);
    } else {
      TCP_CKSUM(pkt->tcp) = 0;
      TCP_CKSUM(pkt->tcp) = get_ipv6_l4_checksum(pkt->ipv6, pkt->l4_hdr_w,
                            get_ipv6_psd_sum(pkt->ipv6));
    }
  }
}

/**
 * Update UDP checksum.
 *
 * @param[in]   pkt     packet for updating.
 */
static inline void
lagopus_update_udp_checksum(struct lagopus_packet *pkt) {
  if (pkt->ether_type == ETHERTYPE_IP) {
    if (HW_UDP_CKSUM_IS_ENABLED) {
      /* calculate udp checksum by NIC, nothing to do. */
      UDP_CKSUM(pkt->udp) = get_ipv4_psd_sum(pkt->ipv4);
    } else {
      UDP_CKSUM(pkt->udp) = 0;
      UDP_CKSUM(pkt->udp) = get_ipv4_l4_checksum(pkt->ipv4, pkt->l4_hdr_w,
                            get_ipv4_psd_sum(pkt->ipv4));
    }
  } else if (pkt->ether_type == ETHERTYPE_IPV6) {
    if (HW_UDP_CKSUM_IS_ENABLED) {
      /* calculate udp checksum by NIC, nothing to do. */
      UDP_CKSUM(pkt->udp) = get_ipv6_psd_sum(pkt->ipv6);
    } else {
      UDP_CKSUM(pkt->udp) = 0;
      UDP_CKSUM(pkt->udp) = get_ipv6_l4_checksum(pkt->ipv6, pkt->l4_hdr_w,
                            get_ipv6_psd_sum(pkt->ipv6));
    }
  }
}

/**
 * Update SCTP checksum.
 *
 * @param[in]   pkt     packet for updating.
 *
 *  SCTP checksum calcuration sample code is described in RFC3309.
 */
static inline void
lagopus_update_sctp_checksum(struct lagopus_packet *pkt) {
  uint32_t crc32c;
  size_t l4_len;
#if BYTE_ORDER == BIG_ENDIAN
  uint8_t byte[4];
#endif

  if (pkt->ether_type == ETHERTYPE_IP) {
    if (HW_SCTP_CKSUM_IS_ENABLED) {
      /* calculate udp checksum by NIC, nothing to do. */
    } else {
      l4_len = (size_t)(IPV4_TLEN(pkt->ipv4) -
                        ((char *)pkt->sctp - (char *)pkt->ipv4));
      SCTP_CKSUM(pkt->sctp) = 0;
      crc32c = calculate_crc32c(0xffffffff,
                                (unsigned char *)pkt->sctp,
                                (unsigned int)l4_len);
      crc32c = ~crc32c;
#if BYTE_ORDER == BIG_ENDIAN
      byte[0] = (crc32c & 0x000000ff);
      byte[1] = (crc32c & 0x0000ff00) >> 8;
      byte[2] = (crc32c & 0x00ff0000) >> 16;
      byte[3] = (crc32c & 0xff000000) >> 24;
      crc32c = (byte[0] << 24) | (byte[1] << 16) | (byte[2]) << 8 | byte[3];
#endif
      SCTP_CKSUM(pkt->sctp) = crc32c;
    }
  } else if (pkt->ether_type == ETHERTYPE_IPV6) {
    if (HW_SCTP_CKSUM_IS_ENABLED) {
    } else {
      l4_len = IPV6_PLEN(pkt->ipv6);
      SCTP_CKSUM(pkt->sctp) = 0;
      crc32c = calculate_crc32c(0xffffffff,
                                (unsigned char *)pkt->sctp,
                                (unsigned int)l4_len);
      crc32c = ~crc32c;
#if BYTE_ORDER == BIG_ENDIAN
      byte[0] = (crc32c & 0x000000ff);
      byte[1] = (crc32c & 0x0000ff00) >> 8;
      byte[2] = (crc32c & 0x00ff0000) >> 16;
      byte[3] = (crc32c & 0xff000000) >> 24;
      crc32c = (byte[0] << 24) | (byte[1] << 16) | (byte[2]) << 8 | byte[3];
#endif
      SCTP_CKSUM(pkt->sctp) = crc32c;
    }
  }
}

/**
 * Update ICMP checksum.
 *
 * @param[in]   pkt     packet for updating.
 */
static inline void
lagopus_update_icmp_checksum(struct lagopus_packet *pkt) {
  ICMP_CKSUM(pkt->icmp) = 0;
  ICMP_CKSUM(pkt->icmp) = get_ipv4_l4_checksum(pkt->ipv4, pkt->l4_hdr_w, 0);
}

/**
 * Update ICMPv6 checksum.
 *
 * @param[in]   pkt     packet for updating.
 */
static inline void
lagopus_update_icmpv6_checksum(struct lagopus_packet *pkt) {
  ICMP_CKSUM(pkt->icmp) = 0;
  ICMP_CKSUM(pkt->icmp) = get_ipv6_l4_checksum(pkt->ipv6, pkt->l4_hdr_w,
                          get_ipv6_psd_sum(pkt->ipv6));
}

/**
 * Update IP header checksum and update TCP/UDP/SCTP/ICMP checksum.
 *
 * @param[in]   pkt     packet for updating.
 */
static inline void
lagopus_update_ipv4_checksum(struct lagopus_packet *pkt) {
  if (HW_IP_CKSUM_IS_ENABLED) {
    /* calculate ip checksum by NIC, nothing to do. */
  } else {
    IPV4_CSUM(pkt->ipv4) = 0;
    IPV4_CSUM(pkt->ipv4) = get_ipv4_cksum(pkt->l3_hdr_w);
  }
  switch (IPV4_PROTO(pkt->ipv4)) {
    case IPPROTO_TCP:
      lagopus_update_tcp_checksum(pkt);
      break;
    case IPPROTO_UDP:
      lagopus_update_udp_checksum(pkt);
      break;
    case IPPROTO_SCTP:
      lagopus_update_sctp_checksum(pkt);
      break;
    case IPPROTO_ICMP:
      lagopus_update_icmp_checksum(pkt);
      break;
    default:
      break;
  }
}

/**
 * Update IPv6 header checksum and TCP/UDP/SCTP/ICMPv6 checksum.
 *
 * @param[in]   pkt     packet for updating.
 */
static inline void
lagopus_update_ipv6_checksum(struct lagopus_packet *pkt) {
  if (pkt->proto != NULL) {
    switch (*pkt->proto) {
      case IPPROTO_TCP:
        lagopus_update_tcp_checksum(pkt);
        break;
      case IPPROTO_UDP:
        lagopus_update_udp_checksum(pkt);
        break;
      case IPPROTO_SCTP:
        lagopus_update_sctp_checksum(pkt);
        break;
      case IPPROTO_ICMPV6:
        lagopus_update_icmpv6_checksum(pkt);
      default:
        break;
    }
  }
}

#endif /* SRC_DATAPLANE_OFPROTO_CSUM_H_ */
