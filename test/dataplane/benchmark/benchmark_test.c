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

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/queue.h>

#include "unity.h"
#include "lagopus/datastore/bridge.h"
#include "lagopus/dp_apis.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowdb.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/ofcache.h"
#include "pktbuf.h"
#include "packet.h"
#include "mbtree.h"
#include "thtable.h"

#include "datapath_test_misc.h"

static struct bridge *bridge;
static struct flowcache *flowcache;
bool loop;

enum {
  TYPE_NULL = 0,
  TYPE_FLOWINFO,
  TYPE_MBTREE,
  TYPE_THTABLE,
  TYPE_FLOWCACHE,
  TYPE_MAX
};

#if 0
#define TYPE_START TYPE_THTABLE
#define TYPE_END   (TYPE_THTABLE + 1)
#else
#define TYPE_START TYPE_NULL
#define TYPE_END   TYPE_MAX
#endif

#define IPADDR(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

const char *
get_type_str(int type) {
  const char *str;

  switch (type) {
    case TYPE_NULL:
      str = "null";
      break;
    case TYPE_FLOWINFO:
      str = "flowinfo";
      break;
    case TYPE_MBTREE:
      str = "mbtree";
      break;
    case TYPE_THTABLE:
      str = "thtable";
      break;
    case TYPE_FLOWCACHE:
      str = "flowcache";
      break;
    default:
      str = "unknown";
      break;
  }
  return str;
}

static void
port_create(int start, int end) {
  char *str;
  int i;

  for (i = start; i < end + 1; i++) {
    asprintf(&str, "port%d", i);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL(dp_port_create(str), LAGOPUS_RESULT_OK);
    free(str);
  }
}

static void
port_attach(const char *br, int start, int end) {
  char *str;
  int i;

  for (i = start; i < end + 1; i++) {
    asprintf(&str, "port%d", i);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL(dp_bridge_port_set(br, str, i + 1), LAGOPUS_RESULT_OK);
    free(str);
  }
}

static void
port_detach(const char *br, int start, int end) {
  char *str;
  int i;

  for (i = start; i < end + 1; i++) {
    asprintf(&str, "port%d", i);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL(dp_bridge_port_unset(br, str), LAGOPUS_RESULT_OK);
    free(str);
  }
}

static void
port_destroy(int start, int end) {
  char *str;
  int i;

  for (i = start; i < end + 1; i++) {
    asprintf(&str, "port%d", i);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL(dp_port_destroy(str), LAGOPUS_RESULT_OK);
    free(str);
  }
}

#define NUMBER_OF_PORTS 8

#define PORT_START 0
#define PORT_END   (PORT_START + NUMBER_OF_PORTS - 1)

void
setUp(void) {
  datastore_bridge_info_t info;

  printf("\n");
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  flowinfo_init();

  /* setup bridge and port */
  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_bridge_create("br0", &info), LAGOPUS_RESULT_OK);
  port_create(PORT_START, PORT_END);
  port_attach("br0", PORT_START, PORT_END);
  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL(bridge);
  flowcache = init_flowcache(FLOWCACHE_HASHMAP_NOLOCK);
  TEST_ASSERT_NOT_NULL(flowcache);
}

void
tearDown(void) {
  port_detach("br0", PORT_START, PORT_END);
  port_destroy(PORT_START, PORT_END);
  TEST_ASSERT_EQUAL(dp_bridge_destroy("br0"), LAGOPUS_RESULT_OK);
  bridge = NULL;
  fini_flowcache(flowcache);

  dp_api_fini();
}

void
print_flowcache_stats(void) {
  struct ofcachestat st;

  get_flowcache_statistics(flowcache, &st);
  printf("*** flowcache stats: entry:%u, hit:%u, miss:%u, ratio %3.2f%%\n",
         st.nentries, st.hit, st.miss,
         (double)st.hit / (double)(st.hit + st.miss) * 100.0);
}

void
flow_add(struct match_list *match_list) {
  struct ofp_flow_mod flow_mod;
  struct instruction_list instruction_list;
  struct ofp_error error;
  lagopus_result_t rv;

  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;
  flow_mod.idle_timeout = 0;
  flow_mod.hard_timeout = 0;

  rv = flowdb_flow_add(bridge, &flow_mod, match_list, &instruction_list,
                       &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
any_flow_add(int n, ...) {
  va_list ap;

  struct match_list match_list;
  int match_type;
  uint32_t match_arg, match_mask;
  int nbytes, i;

  TAILQ_INIT(&match_list);
  va_start(ap, n);
  for (i = 0; i < n; i++) {
    nbytes = va_arg(ap, int);
    match_type = va_arg(ap, int);
    match_arg = va_arg(ap, uint32_t);
    if (match_type & 1) {
      match_mask = va_arg(ap, uint32_t);
      switch (nbytes) {
        case 8:
          add_match(&match_list, nbytes, match_type,
                    (match_arg >> 24) & 0xff,
                    (match_arg >> 16) & 0xff,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff,
                    (match_mask >> 24) & 0xff,
                    (match_mask >> 16) & 0xff,
                    (match_mask >>  8) & 0xff,
                    match_mask & 0xff);
          break;

        case 4:
          add_match(&match_list, nbytes, match_type,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff,
                    (match_mask >>  8) & 0xff,
                    match_mask & 0xff);
          break;

        case 2:
          add_match(&match_list, nbytes, match_type,
                    match_arg & 0xff,
                    match_mask & 0xff);
          break;
      }
    } else {
      switch (nbytes) {
        case 4:
          add_match(&match_list, nbytes, match_type,
                    (match_arg >> 24) & 0xff,
                    (match_arg >> 16) & 0xff,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff);
          break;

        case 2:
          add_match(&match_list, nbytes, match_type,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff);
          break;

        case 1:
          add_match(&match_list, nbytes, match_type,
                    match_arg & 0xff);
          break;
      }
    }
  }
  flow_add(&match_list);
  va_end(ap);
}

void
flow_all_delete(void) {
  struct match_list match_list;
  struct ofp_flow_mod flow_mod;
  struct ofp_error error;
  struct flow_list *flow_list;
  int i, nflow;

  TAILQ_INIT(&match_list);
  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TEST_ASSERT_NOT_NULL(table_lookup(bridge->flowdb, 0));
  flow_list = table_lookup(bridge->flowdb, 0)->flow_list;
  TEST_ASSERT_NOT_NULL(flow_list);
  nflow = flow_list->nflow;
  flowdb_flow_delete(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_EQUAL(flow_list->nflow, 0);
  for (i = 0; i < nflow; i++) {
    TEST_ASSERT_NULL(flow_list->flows[i]);
  }
}

void
in_port_flow_add(uint32_t port_start, uint32_t port_end) {
  uint32_t port;

  if (port_start <= port_end) {
    for (port = port_start; port <= port_end; port++) {
      any_flow_add(1,
                   4, OFPXMT_OFB_IN_PORT << 1, port);
    }
  } else {
    for (port = port_start; port >= port_end; port--) {
      any_flow_add(1,
                   4, OFPXMT_OFB_IN_PORT << 1, port);
    }
  }
}

void
mpls_flow_add(uint32_t in_port, uint32_t label_start, uint32_t label_end) {
  uint32_t label;

  if (label_start <= label_end) {
    for (label = label_start; label <= label_end; label++) {
      any_flow_add(3,
                   4, OFPXMT_OFB_IN_PORT << 1, in_port,
                   2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_MPLS,
                   4, OFPXMT_OFB_MPLS_LABEL << 1, label);
    }
  } else {
    for (label = label_start; label >= label_end; label--) {
      any_flow_add(3,
                   4, OFPXMT_OFB_IN_PORT << 1, in_port,
                   2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_MPLS,
                   4, OFPXMT_OFB_MPLS_LABEL << 1, label);
    }
  }
}

void
ipaddr_flow_add(int type, uint32_t in_port,
                uint32_t ip_start, uint32_t ip_end, int prefix) {
  uint32_t ip, mask, count;

  count = (1 << (32 - prefix));
  mask = ~(count - 1);
  if (ip_start <= ip_end) {
    for (ip = ip_start; ip <= ip_end; ip += count) {
      if (prefix == 32) {
        any_flow_add(3,
                     4, OFPXMT_OFB_IN_PORT << 1, in_port,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     4, type << 1, ip);
      } else {
        any_flow_add(3,
                     4, OFPXMT_OFB_IN_PORT << 1, in_port,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     8, (type << 1) + 1, ip, mask);
      }
    }
  } else {
    for (ip = ip_start; ip >= ip_end; ip--) {
      if (prefix == 32) {
        any_flow_add(3,
                     4, OFPXMT_OFB_IN_PORT << 1, in_port,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     4, type << 1, ip);
      } else {
        any_flow_add(3,
                     4, OFPXMT_OFB_IN_PORT << 1, in_port,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     8, (type << 1) + 1, ip, mask);
      }
    }
  }
}

void
ipsrc_flow_add(uint32_t in_port,
               uint32_t ip_start, uint32_t ip_end, int prefix) {
  ipaddr_flow_add(OFPXMT_OFB_IPV4_SRC, in_port, ip_start, ip_end, prefix);
}

void
ipdst_flow_add(uint32_t in_port,
               uint32_t ip_start, uint32_t ip_end, int prefix) {
  ipaddr_flow_add(OFPXMT_OFB_IPV4_DST, in_port, ip_start, ip_end, prefix);
}

void
sigalrm_handler(int sig) {
  (void) sig;

  loop = false;
}

void
set_timer(time_t sec) {
  struct itimerval newit, oldit;

  signal(SIGALRM, sigalrm_handler);
  newit.it_interval.tv_sec = 0;
  newit.it_interval.tv_usec = 0;
  newit.it_value.tv_sec = sec;
  newit.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &newit, &oldit);
}

struct lagopus_packet *
build_packet(uint32_t in_port, int packet_length) {
  struct lagopus_packet *pkt;
  struct port *port;

  /* prepare packet. */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  pkt->table_id = 0;
  pkt->cache = NULL;
  OS_M_APPEND(PKT2MBUF(pkt), packet_length);
  port = port_lookup(&bridge->ports, in_port);
  TEST_ASSERT_NOT_NULL(port);
  lagopus_packet_init(pkt, PKT2MBUF(pkt), port);

  return pkt;
}

struct lagopus_packet *
build_mpls_packet(uint32_t in_port, uint32_t mpls_label) {
  struct lagopus_packet *pkt;
  struct port *port;
  uint8_t *p;
  const int packet_length = 64;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  pkt->table_id = 0;
  pkt->cache = NULL;
  OS_M_APPEND(PKT2MBUF(pkt), packet_length);
  p = OS_MTOD(PKT2MBUF(pkt), uint8_t *);
  p[12] = (ETHERTYPE_MPLS >> 8) & 0xff;
  p[13] = ETHERTYPE_MPLS & 0xff;
  p[14] = (mpls_label >> 12) & 0xff;
  p[15] = (mpls_label >> 4) & 0xff;
  p[16] = (mpls_label << 4) & 0xff;
  p[16] |= 1; /* BOS */
  p[17] = 64; /* TTL */
  p[18] = 0x00;
  port = port_lookup(&bridge->ports, in_port);
  TEST_ASSERT_NOT_NULL(port);
  lagopus_packet_init(pkt, PKT2MBUF(pkt), port);

  return pkt;
}

struct lagopus_packet *
build_ip_packet(uint32_t in_port, uint32_t ipsrc, uint32_t ipdst,
                uint8_t proto) {
  struct lagopus_packet *pkt;
  struct port *port;
  uint8_t *p;
  const int packet_length = 64;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  pkt->table_id = 0;
  pkt->cache = NULL;
  OS_M_APPEND(PKT2MBUF(pkt), packet_length);
  p = OS_MTOD(PKT2MBUF(pkt), uint8_t *);
  p[12] = (ETHERTYPE_IP >> 8) & 0xff;
  p[13] = ETHERTYPE_IP & 0xff;
  p[14] = 0x45;
  p[23] = proto;
  p[26] = (ipsrc >> 24) & 0xff;
  p[27] = (ipsrc >> 16) & 0xff;
  p[28] = (ipsrc >> 8) & 0xff;
  p[29] = ipsrc & 0xff;
  p[30] = (ipdst >> 24) & 0xff;
  p[31] = (ipdst >> 16) & 0xff;
  p[32] = (ipdst >> 8) & 0xff;
  p[33] = ipdst & 0xff;
  port = port_lookup(&bridge->ports, in_port);
  TEST_ASSERT_NOT_NULL(port);
  lagopus_packet_init(pkt, PKT2MBUF(pkt), port);

  return pkt;
}

void
cache_enable(struct lagopus_packet *pkt) {
  pkt->cache = flowcache;
}

void
cache_disable(struct lagopus_packet *pkt) {
  pkt->cache = NULL;
}

void
destroy_packet(struct lagopus_packet *pkt) {
  lagopus_packet_free(pkt);
}

void
flow_benchmark(int type,
               struct lagopus_packet *pkts[], size_t npkts, time_t sec) {
  struct flowdb *flowdb;
  struct table *table;
  struct flow *flow;
  struct lagopus_packet *pkt;
  const struct cache_entry *cache_entry;
  uint64_t lookup_count, match_count;
  size_t i;

  loop = true;
  lookup_count = 0;
  match_count = 0;

  if (type == TYPE_MBTREE) {
    flowdb = pkts[0]->in_port->bridge->flowdb;
    table = table_lookup(flowdb, pkts[0]->table_id);
    if (table->flow_list->type == 0) {
      cleanup_mbtree(table->flow_list);
      build_mbtree(table->flow_list);
    }
  }
  if (type == TYPE_THTABLE) {
    flowdb = pkts[0]->in_port->bridge->flowdb;
    table = table_lookup(flowdb, pkts[0]->table_id);
    thtable_update(table->flow_list);
    TEST_ASSERT_NOT_NULL(table->flow_list->thtable);
  }
  if (type == TYPE_FLOWCACHE) {
    for (i = 0; i < npkts; i++) {
      cache_enable(pkts[i]);
    }
  }

  set_timer(sec);
  /* lookup loop */
  while (loop == true) {
    for (i = 0; i < npkts && loop == true; i++) {
      pkt = pkts[i];
      flowdb = pkt->bridge->flowdb;
      table = table_lookup(flowdb, pkt->table_id);
      switch (type) {
        case TYPE_NULL:
          flow = (void *)1;
          break;
        case TYPE_FLOWINFO:
          flow = lagopus_find_flow(pkt, table);
          break;
        case TYPE_MBTREE:
          flow = find_mbtree(pkt, table->flow_list);
          break;
        case TYPE_THTABLE:
          flow = thtable_match(pkt, table->flow_list->thtable);
          break;
        case TYPE_FLOWCACHE:
          cache_entry = cache_lookup(pkt->cache, pkt);
          if (unlikely(cache_entry == NULL)) {
            flow = lagopus_find_flow(pkt, table);
            if (flow != NULL && pkt->cache != NULL) {
              register_cache(pkt->cache, pkt->hash64, 1, &flow);
            }
          }
          break;
      }
      lookup_count++;
      if (flow != NULL) {
        match_count++;
      }
    }
  }
  TEST_ASSERT_NOT_EQUAL(lookup_count, 0);
  /*TEST_ASSERT_EQUAL(lookup_count, match_count);*/
  if (match_count == 0) {
    printf("warning: match count == 0\n");
  }
  if ((double)lookup_count / (double)sec / 1000000.0 >= 0.1) {
    printf("*** %s: %3.2fM lookup/sec\n",
           get_type_str(type),
           (double)lookup_count / (double)sec / 1000000.0);
  } else if ((double)lookup_count / (double)sec / 1000.0 >= 0.1) {
    printf("*** %s: %3.2fK lookup/sec\n",
           get_type_str(type),
           (double)lookup_count / (double)sec / 1000.0);
  } else {
    printf("*** %s: %3.2f lookup/sec\n",
           get_type_str(type),
           (double)lookup_count / (double)sec);
  }
}

void
test_1_entry_flow_benchmark(void) {
  struct lagopus_packet *pkt;
  int type;

  printf("******** 1 entry, 1 flow, port match ******************\n");
  for (type = TYPE_START; type < TYPE_END; type++) {
    in_port_flow_add(1, 1);
    pkt = build_packet(1, 64);
    flow_benchmark(type, &pkt, 1, 1);
    destroy_packet(pkt);
    flow_all_delete();
  }
}

void
test_2_1_entries_flow_benchmark(void) {
  struct lagopus_packet *pkt;
  int type;

  printf("******** 2 entry, 1 flow, port match ******************\n");
  for (type = TYPE_START; type < TYPE_END; type++) {
    in_port_flow_add(2, 1);
    pkt = build_packet(1, 64);
    flow_benchmark(type, &pkt, 1, 1);
    destroy_packet(pkt);
    flow_all_delete();
  }
}

void
test_256_1_entries_flow_benchmark(void) {
  struct lagopus_packet *pkt;
  int type;

  printf("****** 256 entry, 1 flow, port match ******************\n");
  for (type = TYPE_START; type < TYPE_END; type++) {
    in_port_flow_add(256, 1);
    pkt = build_packet(1, 64);
    flow_benchmark(type, &pkt, 1, 1);
    destroy_packet(pkt);
    flow_all_delete();
  }
}
void
test_MPLS_1_entries_1_flow_benchmark(void) {
  struct lagopus_packet *pkt;
  size_t i;
  int type;

  printf("******* 1 entry, 1 flow, MPLS match ******************\n");
  for (type = TYPE_START; type < TYPE_END; type++) {
    mpls_flow_add(1, 1, 1);
    pkt = build_mpls_packet(1, 1);
    flow_benchmark(type, &pkt, 1, 3);
    destroy_packet(pkt);
    flow_all_delete();
  }
}

void
MPLS_1M_entries_n_flow_benchmark(size_t n) {
  struct lagopus_packet *pkt[n];
  size_t i;
  int type;

  for (type = TYPE_START; type < TYPE_END; type++) {
    mpls_flow_add(1, 1000 * 1000, 1);
    for (i = 0; i < n; i++) {
      pkt[i] = build_mpls_packet(1, i + 1);
    }
    flow_benchmark(type, pkt, n, 3);
    for (i = 0; i < n; i++) {
      destroy_packet(pkt[i]);
    }
    flow_all_delete();
  }
}

void
test_MPLS_1M_entries_flow_benchmark(void) {
  printf("******* 1M entry, 1 flow, MPLS match ******************\n");
  MPLS_1M_entries_n_flow_benchmark(1);
}

void
test_MPLS_1M_entries_1000_flow_benchmark(void) {
  printf("******* 1M entry, 1000 flow, MPLS match ***************\n");
  MPLS_1M_entries_n_flow_benchmark(1000);
}

void
test_MPLS_1M_entries_1M_flow_benchmark(void) {
  printf("******* 1M entry, 1M flow, MPLS match *****************\n");
  MPLS_1M_entries_n_flow_benchmark(1000 * 1000);
}

void
IPV4_SRC_65536_entries_flow_benchmark(size_t n) {
  struct lagopus_packet *pkt[n];
  size_t i;
  int type;

  for (type = TYPE_START; type < TYPE_END; type++) {
    ipsrc_flow_add(1, IPADDR(192,168,0,0), IPADDR(192,168,255,255), 32);
    for (i = 0; i < n; i++) {
      pkt[i] = build_ip_packet(1, IPADDR(192,168,1,i), IPADDR(192,168,1,2), 0);
    }
    flow_benchmark(type, pkt, n, 3);
    for (i = 0; i < n; i++) {
      destroy_packet(pkt[i]);
    }
    flow_all_delete();
  }
}

void
test_IPV4_SRC_65536_entries_1_flow_benchmark(void) {
  printf("***** 65536 entry, 1 flow, IPv4src match **************\n");
  IPV4_SRC_65536_entries_flow_benchmark(1);
}

void
test_IPV4_SRC_65536_entries_256_flow_benchmark(void) {
  printf("***** 65536 entry, 256 flow, IPv4src match ************\n");
  IPV4_SRC_65536_entries_flow_benchmark(256);
}

void
IPV4_DST_65536_entries_flow_benchmark(size_t n) {
  struct lagopus_packet *pkt[n];
  size_t i;
  int type;

  for (type = TYPE_START; type < TYPE_END; type++) {
    ipdst_flow_add(1, IPADDR(192,168,0,0), IPADDR(192,168,255,255), 32);
    for (i = 0; i < n; i++) {
      pkt[i] = build_ip_packet(1, IPADDR(192,168,1,2), IPADDR(192,168,1,i), 0);
    }
    flow_benchmark(type, pkt, n, 3);
    for (i = 0; i < n; i++) {
      destroy_packet(pkt[i]);
    }
    flow_all_delete();
  }
}

void
test_IPV4_DST_65536_entries_1_flow_benchmark(void) {
  printf("***** 65536 entry, 1 flow, IPv4dst match **************\n");
  IPV4_DST_65536_entries_flow_benchmark(1);
}

void
test_IPV4_DST_65536_entries_256_flow_benchmark(void) {
  printf("***** 65536 entry, 256 flow, IPv4dst match ************\n");
  IPV4_DST_65536_entries_flow_benchmark(256);
}

void
test_mixed_entries_1M_flow_benchmark(void) {
  size_t i;
  int type;
  const int n = 1000 * 1000;
  struct lagopus_packet *pkt[n];

  printf("***** 65536 * 3 + 1M entry, 1M flow, MPLS match ************\n");
  for (type = TYPE_START; type < TYPE_END; type++) {
    in_port_flow_add(2, 65537);
    ipsrc_flow_add(1, IPADDR(192,168,0,0), IPADDR(192,168,255,255), 32);
    ipdst_flow_add(1, IPADDR(192,168,0,0), IPADDR(192,168,255,255), 32);
    mpls_flow_add(1, 1000 * 1000, 1);
    for (i = 0; i < n; i++) {
      pkt[i] = build_mpls_packet(1, i + 1);
    }
    flow_benchmark(type, pkt, n, 3);
    for (i = 0; i < n; i++) {
      destroy_packet(pkt[i]);
    }
    flow_all_delete();
  }
}
