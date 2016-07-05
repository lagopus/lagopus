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

#include <rte_config.h>
#include <rte_mbuf.h>

#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/port.h"
#include "lagopus/bridge.h"
#include "dpdk/dpdk.h"
#include "dpdk/pktbuf.h"
#include "ofproto/packet.h"

#include "match_pkts.h"

#define DPID 0x1LL
#define IN_PORT 1

lagopus_result_t
lagopus_packet_init(struct lagopus_packet *pkt, void *m, struct port *port);
lagopus_result_t
lagopus_match_and_action(struct lagopus_packet *pkt);

static struct table *table = NULL;

lagopus_result_t
setup(void *pkts, size_t size) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  struct rte_mbuf **mbufs;
  struct lagopus_packet *pkt = NULL;
  struct bridge *bridge = NULL;
  struct port port = {0};

  if (pkts != NULL) {
    mbufs = pkts;

    bridge = dp_bridge_lookup_by_dpid(DPID);
    if (bridge != NULL) {
      port.ofp_port.port_no = IN_PORT;
      port.bridge = bridge;

      for (i = 0; i < size; i++) {
        pkt = MBUF2PKT(mbufs[i]);
        lagopus_packet_init(pkt, mbufs[i], &port);
        table = table_lookup(bridge->flowdb, pkt->table_id);

        if (table == NULL) {
          ret = LAGOPUS_RESULT_NOT_FOUND;
          lagopus_perror(ret);
          goto done;
        }
      }

      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NOT_FOUND;
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

 done:
  return ret;
}

lagopus_result_t
teardown(void *pkts, size_t size) {
  (void) pkts;
  (void) size;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
match_pkts(void *pkts, size_t size) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  struct rte_mbuf **mbufs;
  struct lagopus_packet *pkt = NULL;
  struct flow *flow = NULL;

  if (pkts != NULL) {
    mbufs = pkts;
    rte_prefetch1(rte_pktmbuf_mtod(mbufs[0],
                                   unsigned char *));
    rte_prefetch0(mbufs[1]);
    for (i = 0; i < size; i++) {
      if (likely(i < size - 1)) {
        rte_prefetch1(rte_pktmbuf_mtod(mbufs[i + 1],
                                       unsigned char *));
      }
      if (likely(i < size - 2)) {
        rte_prefetch0(mbufs[i + 2]);
      }

      pkt = MBUF2PKT(mbufs[i]);
      flow = lagopus_find_flow(pkt, table);
      if (flow != NULL) {
        fprintf(stdout, "match!!\n");
        ret = LAGOPUS_RESULT_OK;
      } else {
        fprintf(stdout, "unmatch!!\n");
        ret = LAGOPUS_RESULT_OK;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}
