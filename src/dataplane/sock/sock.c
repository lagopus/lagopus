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

/**
 *      @file   sock.c
 *      @brief  Dataplane driver use with raw socket
 */

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <poll.h>
#include <pthread.h>
#include <err.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/socket.h>
//#include <net/ethernet.h>
#include <net/if.h>
#include <pthread.h>

#include <linux/if_packet.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "lagopus/dp_apis.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofcache.h"
#include "lagopus/interface.h"
#include "pktbuf.h"
#include "packet.h"
#include "pcap.h"

struct lagopus_packet *
alloc_lagopus_packet(void) {
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m) + sizeof(*pkt));
  if (m == NULL) {
    lagopus_msg_error("mbuf alloc failed\n");
    return NULL;
  }
  pkt = (struct lagopus_packet *)&m[1];
  m->data = &m->dat[128];
  pkt->mbuf = m;

  return pkt;
}

void
sock_m_free(OS_MBUF *m) {
  if (m->refcnt-- <= 0) {
    free(m);
  }
}

void
lagopus_packet_free(struct lagopus_packet *pkt) {
  OS_M_FREE(pkt->mbuf);
}

int
lagopus_send_packet_normal(__UNUSED struct lagopus_packet *pkt,
                           __UNUSED uint32_t portid) {
  /* not implemented yet */
  return 0;
}

struct lagopus_packet *
copy_packet(struct lagopus_packet *src_pkt) {
  OS_MBUF *mbuf;
  struct lagopus_packet *pkt;
  size_t pktlen;

  pkt = alloc_lagopus_packet();
  if (pkt == NULL) {
    lagopus_msg_error("alloc_lagopus_packet failed\n");
    return NULL;
  }
  mbuf = pkt->mbuf;
  pktlen = OS_M_PKTLEN(src_pkt->mbuf);
  (void)OS_M_APPEND(mbuf, pktlen);
  memcpy(OS_MTOD(pkt->mbuf, char *), OS_MTOD(src_pkt->mbuf, char *), pktlen);
  pkt->in_port = src_pkt->in_port;
  /* other pkt members are not used in physical output. */
  return pkt;
}

void
lagopus_instruction_experimenter(__UNUSED struct lagopus_packet *pkt,
                                 __UNUSED uint32_t exp_id) {
  /* writing your own instruction */
}

int
lagopus_meter_packet(__UNUSED struct lagopus_packet *pkt,
                     __UNUSED struct meter *meter,
                     __UNUSED uint8_t *prec_level) {
  /* XXX not implemented */
  return 0;
}

void
lagopus_meter_init(void) {
  /* XXX not implemented */
}

lagopus_result_t
lagopus_dataplane_init(int argc, const char *const argv[]) {
  return rawsock_dataplane_init(argc, argv);
}

void
dp_get_flowcache_statistics(struct bridge *bridge, struct ofcachestat *st) {
  st->nentries = 0;
  st->hit = 0;
  st->miss = 0;
  /* not implemented yet */
}

void
dataplane_usage(__UNUSED FILE *fp) {
  /* so far, nothing additional options. */
}
