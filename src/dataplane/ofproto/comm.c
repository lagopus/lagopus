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
 *      @file   comm.c
 *      @brief  Dataplane communicator thread functions.
 */

#include "lagopus_config.h"

#ifdef HAVE_DPDK
#include <rte_config.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_launch.h>
#endif /* HAVE_DPDK */

#include "lagopus_apis.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/flowdb.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofcache.h"
#include "lagopus/ofp_bridge.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/dp_apis.h"
#ifdef USE_MBTREE
#include "lagopus/flowdb.h"
#endif /* USE_MBTREE */

#include "pktbuf.h"
#include "packet.h"
#include "thread.h"
#include "mbtree.h"
#include "lock.h"

#include "callback.h"

#ifdef HAVE_DPDK
#include "dpdk.h"
#endif /* HAVE_DPDK */

static struct flowcache *cache;

#define PUT_TIMEOUT 100LL * 1000LL * 1000LL

lagopus_result_t
dp_process_event_data(uint64_t dpid, struct eventq_data *data) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct bridge *bridge;

#ifdef HAVE_DPDK
  if (!app.no_cache) {
    cache = init_flowcache(app.kvs_type);
  } else {
    cache = NULL;
  }
#else
  cache = NULL;
#endif /* HAVE_DPDK */

  lagopus_msg_debug(10, "get item. %p\n", data);
  if (data == NULL) {
    lagopus_msg_warning("process_event_dataq_entry arg is NULL\n");
    rv = LAGOPUS_RESULT_INVALID_ARGS;
    goto done;
  }

  flowdb_check_update(NULL);
  flowdb_rdlock(NULL);
  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    struct eventq_data *reply;
    struct lagopus_packet *pkt;
    struct port *port;
    struct pbuf *pbuf;
    uint16_t data_len;

    switch (data->type) {
      case LAGOPUS_EVENTQ_PACKET_OUT:
        pbuf = data->packet_out.data;
        if (pbuf == NULL) {
          break;
        }
        /* create packet structure and send to specified port. */
        pkt = alloc_lagopus_packet();
        if (pkt != NULL) {
          struct port cport;
          struct ofp_packet_out *ofp_packet_out;

          port = NULL;
          ofp_packet_out = &data->packet_out.ofp_packet_out;
          if (ofp_packet_out->in_port != OFPP_CONTROLLER &&
              ofp_packet_out->in_port != OFPP_ANY) {
            port = port_lookup(&bridge->ports,
                               ofp_packet_out->in_port);
          }
          if (port == NULL) {
            /**
             * silly hack.
             * bridge should have complete struct port for controller port.
             */
            port = &cport;
            cport.ifindex = 0xff;
            cport.bridge = bridge;
            cport.ofp_port.port_no = ofp_packet_out->in_port;
          }
          rv = pbuf_length_get(pbuf, &data_len);
          if (rv != LAGOPUS_RESULT_OK) {
            lagopus_msg_error("pbuf_length_get error (%s).\n",
                              lagopus_error_get_string(rv));
            break;
          }

          (void)OS_M_APPEND(PKT2MBUF(pkt), data_len);
          DECODE_GET(OS_MTOD(PKT2MBUF(pkt), char *), data_len);
          lagopus_packet_init(pkt, PKT2MBUF(pkt), port);
          pkt->cache = NULL;
          pkt->hash64 = 0;
          if (lagopus_register_action_hook != NULL) {
            struct action *action;

            TAILQ_FOREACH(action, &data->packet_out.action_list, entry) {
              lagopus_register_action_hook(action);
            }
          }
          /* XXX so far, call from comm thread directly. */
          OS_M_ADDREF(PKT2MBUF(pkt));
          execute_action(pkt, &data->packet_out.action_list);
          lagopus_packet_free(pkt);
        }
        break;

      case LAGOPUS_EVENTQ_BARRIER_REQUEST:
#ifdef USE_MBTREE
        /* rebuild branch tree. */
        {
          struct flowdb *flowdb;
          struct table *table;
          int i;

          flowdb = bridge->flowdb;
          for (i = 0; i < FLOWDB_TABLE_SIZE_MAX; i++) {
            table = flowdb_get_table(flowdb, i);
            if (table != NULL) {
              cleanup_mbtree(table->flow_list);
              build_mbtree(table->flow_list);
            }
          }
        }
#endif /* USE_MBTREE */
#ifdef USE_THTABLE
        /* rebuild tuple hash table. */
        {
          struct flowdb *flowdb;
          struct table *table;
          int i;

          flowdb = bridge->flowdb;
          for (i = 0; i < FLOWDB_TABLE_SIZE_MAX; i++) {
            table = flowdb_get_table(flowdb, i);
            if (table != NULL) {
              thtable_update(table->flow_list);
            }
          }
        }
#endif /* USE_THTABLE */
        /* flush pending requests from OFC, and reply. */
        if (cache != NULL) {
          /* clear my own cache */
          clear_all_cache(cache);
        }
#ifdef HAVE_DPDK
        /* and worker cache */
        clear_worker_flowcache(true);
#endif /* HAVE_DPDK */
        reply = malloc(sizeof(*reply));
        if (reply == NULL) {
          break;
        }
        reply->type = LAGOPUS_EVENTQ_BARRIER_REPLY;
        reply->free = free;
        reply->barrier.req = NULL;
        reply->barrier.xid = data->barrier.xid;
        reply->barrier.channel_id = data->barrier.channel_id;
        (void) dp_eventq_data_put(dpid, &reply, PUT_TIMEOUT);
        break;
      default:
        break;
    }
  } else {
    rv = LAGOPUS_RESULT_INVALID_OBJECT;
  }
  flowdb_rdunlock(NULL);

done:
  return rv;
}
