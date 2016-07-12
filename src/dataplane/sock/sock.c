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
#include <net/if.h>
#include <pthread.h>

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
  OS_M_FREE(PKT2MBUF(pkt));
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

void
dp_get_flowcache_statistics(struct bridge *bridge, struct ofcachestat *st) {
  st->nentries = 0;
  st->hit = 0;
  st->miss = 0;
  /* not implemented yet */
}
