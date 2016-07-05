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

void
setup(void) {
}

void
teardown(void) {
}

#define DPID 0x1LL
#define IN_PORT 1

lagopus_result_t
lagopus_packet_init(struct lagopus_packet *pkt, void *m, struct port *port);
lagopus_result_t
lagopus_match_and_action(struct lagopus_packet *pkt);

lagopus_result_t
match_pkts(void *pkts, size_t size) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  struct rte_mbuf **mbufs;
  struct lagopus_packet *pkt = NULL;
  struct bridge *bridge = NULL;
  struct table *table = NULL;
  struct flow *flow = NULL;
  struct port port = {0};

  if (pkts != NULL) {
    mbufs = pkts;

    bridge = dp_bridge_lookup_by_dpid(DPID);
    if (bridge != NULL) {
      port.ofp_port.port_no = IN_PORT;
      port.bridge = bridge;
      for (i = 0; i < size; i++) {
        pkt = (struct lagopus_packet *)
            (mbufs[i]->buf_addr + APP_DEFAULT_MBUF_LOCALDATA_OFFSET);
        lagopus_packet_init(pkt, mbufs[i], &port);
        table = table_lookup(bridge->flowdb, pkt->table_id);

        if (table != NULL) {
          flow = lagopus_find_flow(pkt, table);
          if (flow != NULL) {
            fprintf(stdout, "match!!\n");
            ret = LAGOPUS_RESULT_OK;
          } else {
            fprintf(stdout, "unmatch!!\n");
            ret = LAGOPUS_RESULT_OK;
          }
        } else {
          ret = LAGOPUS_RESULT_NOT_FOUND;
          lagopus_perror(ret);
          goto done;
        }
      }
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
