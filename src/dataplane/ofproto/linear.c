/*
	Simplified version of flowinfo_basic.c
	Uses a flow_list to find the matching flow rather than flowinfo
 */

#include "lagopus_config.h"

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/queue.h>

#ifndef HAVE_DPDK
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#else
#include <net/if.h>
#include <net/if_ether.h>
#endif /* HAVE_NET_ETHERNET_H */
#endif /* HAVE_DPDK */

#define __FAVOR_BSD
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <openflow.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"

bool is_match(const struct lagopus_packet *, struct flow *);

struct flow *
find_linear(struct lagopus_packet *packet, struct flow_list *flow_list);

bool
is_match(const struct lagopus_packet *packet, struct flow *flow) {
  struct byteoff_match *match;
  uint8_t *base;
  uint32_t bits;
  int off, i, max;

  if (unlikely(packet->ether_type == ETHERTYPE_IPV6)) {
    max = MAX_BASE;
  } else {
    max = OOB2_BASE + 1;
  }

  for (i = 0; i < max; i++) {
    match = &flow->byteoff_match[i];//Bytes in field to be matched
    if (match->bits == 0) {//Skip this field if not matching on this field
      continue;
    }
    base = packet->base[i];
    if (base == NULL) {//Bad packet
      return false;
    }
    off = 0;
    bits = match->bits;
    do {
      if ((bits & 0x0f) != 0) {//Optimization to avoid needlessly checking equality of 0 and 0
        uint32_t b, m, c;

        memcpy(&b, &base[off], sizeof(uint32_t));
        memcpy(&m, &match->masks[off], sizeof(uint32_t));
        memcpy(&c, &match->bytes[off], sizeof(uint32_t));
        if ((b & m) != c) {//If the masked bits of the packet are not equal to the bits of the flow, it is not a match
          return false;
        }
      }
      off += 4;
      bits >>= 4;
    } while (bits != 0);//While there are bits to match on 
  }

  //Lagopus status flags, update packet count and byte count on the flow. May be important for tests
  if ((flow->flags & OFPFF_NO_PKT_COUNTS) == 0) {
    flow->packet_count++;
  }
  if ((flow->flags & OFPFF_NO_BYT_COUNTS) == 0) {
    flow->byte_count += OS_M_PKTLEN(PKT2MBUF(packet));
  }

  return true;
}

struct flow *
find_linear(struct lagopus_packet *packet, struct flow_list *flow_list) {
 
  struct flow *matched;
  int i;

  matched = NULL;

  //This assumes flow_list is sorted in priority order
  for (i = 0; i < flow_list->nflow; i++) {
    struct flow *flow = flow_list->flows[i];

    if (is_match(packet, flow) == true) {
//    if (flow_matches_l_packet(flow, packet) == true) {
      matched = flow;
      break;
    }

  }
  return matched;
}

