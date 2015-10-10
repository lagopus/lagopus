/* %COPYRIGHT% */

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

#include "datapath_test_misc.h"

static struct bridge *bridge;
static struct flowcache *flowcache;
bool loop;

void
setUp(void) {
  datastore_bridge_info_t info;

  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  flowinfo_init();

  /* setup bridge and port */
  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_bridge_create("br0", &info), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port0", 1), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port1", 2), LAGOPUS_RESULT_OK);
  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL(bridge);
  flowcache = init_flowcache(FLOWCACHE_HASHMAP_NOLOCK);
  TEST_ASSERT_NOT_NULL(flowcache);
}

void
tearDown(void) {
  TEST_ASSERT_EQUAL(dp_bridge_port_unset("br0", "port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_unset("br0", "port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_destroy("port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_destroy("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_destroy("br0"), LAGOPUS_RESULT_OK);
  bridge = NULL;
  fini_flowcache(flowcache);

  dp_api_fini();
}

void
print_flowcache_stats(void) {

  printf("flowcache stats: entry:%u, hit:%u, miss:%u, ratio %f%%\n",
         flowcache->nentries, flowcache->hit, flowcache->miss,
         (double)flowcache->hit / (double)(flowcache->hit + flowcache->miss));
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
in_port_flow_add(uint32_t port_start, uint32_t port_end) {
  struct match_list match_list;
  uint32_t port;

  if (port_start <= port_end) {
    for (port = port_start; port <= port_end; port++) {
      any_flow_add(1,
                   4, OFPXMT_OFB_IN_PORT << 1, port);
    }
  } else {
    for (port = port_start; port >= port_end; port--) {
      TAILQ_INIT(&match_list);
      any_flow_add(1,
                   4, OFPXMT_OFB_IN_PORT << 1, port);
    }
  }
}

void
mpls_flow_add(uint32_t label_start, uint32_t label_end) {
  struct match_list match_list;
  uint32_t label;

  if (label_start <= label_end) {
    for (label = label_start; label <= label_end; label++) {
      TAILQ_INIT(&match_list);
      any_flow_add(2,
                   2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_MPLS,
                   4, OFPXMT_OFB_MPLS_LABEL << 1, label);
    }
  } else {
    for (label = label_start; label >= label_end; label--) {
      TAILQ_INIT(&match_list);
      any_flow_add(2,
                   2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_MPLS,
                   4, OFPXMT_OFB_MPLS_LABEL << 1, label);
    }
  }
}

void
ipsrc_flow_add(uint32_t ip_start, uint32_t ip_end, int prefix) {
  struct match_list match_list;
  uint32_t ip, mask, count;

  count = (1 << (32 - prefix));
  mask = ~(count - 1);
  if (ip_start <= ip_end) {
    for (ip = ip_start; ip <= ip_end; ip += count) {
      TAILQ_INIT(&match_list);
      if (prefix == 32) {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     4, OFPXMT_OFB_IPV4_SRC << 1, ip);
      } else {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     8, (OFPXMT_OFB_IPV4_SRC << 1) + 1, ip, mask);
      }
    }
  } else {
    for (ip = ip_start; ip >= ip_end; ip--) {
      TAILQ_INIT(&match_list);
      if (prefix == 32) {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     4, OFPXMT_OFB_IPV4_SRC << 1, ip);
      } else {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     8, (OFPXMT_OFB_IPV4_SRC << 1) + 1, ip, mask);
      }
    }
  }
}

void
ipdst_flow_add(uint32_t ip_start, uint32_t ip_end, int prefix) {
  struct match_list match_list;
  uint32_t ip, mask, count;

  count = (1 << (32 - prefix));
  mask = ~(count - 1);
  if (ip_start <= ip_end) {
    for (ip = ip_start; ip <= ip_end; ip += count) {
      TAILQ_INIT(&match_list);
      if (prefix == 32) {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     4, OFPXMT_OFB_IPV4_DST << 1, ip);
      } else {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     8, (OFPXMT_OFB_IPV4_DST << 1) + 1, ip, mask);
      }
    }
  } else {
    for (ip = ip_start; ip >= ip_end; ip--) {
      TAILQ_INIT(&match_list);
      if (prefix == 32) {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     4, OFPXMT_OFB_IPV4_DST << 1, ip);
      } else {
        any_flow_add(2,
                     2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
                     8, (OFPXMT_OFB_IPV4_DST << 1) + 1, ip, mask);
      }
    }
  }
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
build_packet(uint32_t in_port) {
  struct lagopus_packet *pkt;
  struct port *port;
  const int packet_length = 64;

  /* prepare packet. */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  pkt->table_id = 0;
  pkt->cache = NULL;
  OS_M_APPEND(pkt->mbuf, packet_length);
  port = port_lookup(bridge->ports, in_port);
  TEST_ASSERT_NOT_NULL(port);
  lagopus_packet_init(pkt, pkt->mbuf, port);

  return pkt;
}

struct lagopus_packet *
build_mpls_packet(uint32_t in_port, uint32_t mpls_label) {
  struct lagopus_packet *pkt;
  struct port *port;
  uint8_t *p;
  const int packet_length = 64;

  /* prepare packet. */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  pkt->table_id = 0;
  pkt->cache = NULL;
  OS_M_APPEND(pkt->mbuf, packet_length);
  p = OS_MTOD(pkt->mbuf, uint8_t *);
  p[12] = (ETHERTYPE_MPLS >> 8) & 0xff;
  p[13] = ETHERTYPE_MPLS & 0xff;
  p[14] = (mpls_label >> 12) & 0xff;
  p[15] = (mpls_label >> 4) & 0xff;
  p[16] = (mpls_label << 4) & 0xff;
  port = port_lookup(bridge->ports, in_port);
  TEST_ASSERT_NOT_NULL(port);
  lagopus_packet_init(pkt, pkt->mbuf, port);

  return pkt;
}

struct lagopus_packet *
build_ip_packet(uint32_t in_port, uint32_t ipsrc, uint32_t ipdst,
                uint8_t proto) {
  struct lagopus_packet *pkt;
  struct port *port;
  uint8_t *p;
  const int packet_length = 64;

  /* prepare packet. */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  pkt->table_id = 0;
  pkt->cache = NULL;
  OS_M_APPEND(pkt->mbuf, packet_length);
  p = OS_MTOD(pkt->mbuf, uint8_t *);
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
  port = port_lookup(bridge->ports, in_port);
  TEST_ASSERT_NOT_NULL(port);
  lagopus_packet_init(pkt, pkt->mbuf, port);

  return pkt;
}

void
cache_enable(struct lagopus_packet *pkt) {
  pkt->cache = flowcache;
}

void
destroy_packet(struct lagopus_packet *pkt) {
  lagopus_packet_free(pkt);
}

void
flow_benchmark(struct lagopus_packet *pkts[], size_t npkts, time_t sec) {
  struct flowdb *flowdb;
  struct table *table;
  struct flow *flow;
  struct lagopus_packet *pkt;
  const struct cache_entry *cache_entry;
  uint64_t lookup_count, match_count;

  loop = true;
  lookup_count = 0;
  match_count = 0;

#ifdef USE_MBTREE
  flowdb = pkts[0]->in_port->bridge->flowdb;
  table = table_lookup(flowdb, pkts[0]->table_id);
  if (table->flow_list->type == 0) {
    cleanup_mbtree(table->flow_list);
    build_mbtree(table->flow_list);
  }
#endif /* USE_MBTREE */

  set_timer(sec);
  /* lookup loop */
  while (loop == true) {
    size_t i;

    for (i = 0; i < npkts && loop == true; i++) {
      pkt = pkts[i];
      flowdb = pkt->in_port->bridge->flowdb;
      table = table_lookup(flowdb, pkt->table_id);
      cache_entry = cache_lookup(pkt->cache, pkt);
      if (likely(cache_entry != NULL)) {
        lookup_count++;
        match_count++;
      } else {
        flow = lagopus_find_flow(pkt, table);
        lookup_count++;
        if (flow != NULL) {
          match_count++;
          if (pkt->cache != NULL) {
            register_cache(pkt->cache, pkt->hash64, 1, &flow);
          }
        }
      }
    }
  }
  printf("%dsec lookup_count %d\n", sec, lookup_count);
  printf("%dsec match_count %d\n", sec, match_count);
  TEST_ASSERT_NOT_EQUAL(lookup_count, 0);
  TEST_ASSERT_NOT_EQUAL(match_count, 0);
}

void
test_1_entry_flow_benchmark(void) {
  struct lagopus_packet *pkt;

  printf("\n*** 1 entry (in_port = 1)\n");
  in_port_flow_add(1, 1);
  pkt = build_packet(1);
  flow_benchmark(&pkt, 1, 1);
  printf("\n*** cached 1 entry (in_port = 1)\n");
  cache_enable(pkt);
  flow_benchmark(&pkt, 1, 1);
  print_flowcache_stats();
  destroy_packet(pkt);
}

void
test_2_1_entries_flow_benchmark(void) {
  struct lagopus_packet *pkt;

  printf("\n*** 2 entries (in_port = 2 -> 1)\n");
  in_port_flow_add(256, 1);
  pkt = build_packet(1);
  flow_benchmark(&pkt, 1, 1);
  printf("\n*** cached 2 entries (in_port = 2 -> 1)\n");
  cache_enable(pkt);
  flow_benchmark(&pkt, 1, 1);
  print_flowcache_stats();
  destroy_packet(pkt);
}

void
test_256_1_entries_flow_benchmark(void) {
  struct lagopus_packet *pkt;

  printf("\n*** 256 entries (in_port = 256 -> 1)\n");
  in_port_flow_add(256, 1);
  pkt = build_packet(1);
  flow_benchmark(&pkt, 1, 1);
  printf("\n*** cached 256 entries (in_port = 256 -> 1)\n");
  cache_enable(pkt);
  flow_benchmark(&pkt, 1, 1);
  print_flowcache_stats();
  destroy_packet(pkt);
}

void
MPLS_1M_entries_n_flow_benchmark(size_t n) {
  struct lagopus_packet *pkt[n];
  size_t i;

  printf("\n*** 1M entries %u flow(s) (MPLS label = 1M -> 1)\n", n);
  mpls_flow_add(1000 * 1000, 1);
  for (i = 0; i < n; i++) {
    pkt[i] = build_mpls_packet(1, i + 1);
  }
  flow_benchmark(pkt, n, 1);
  printf("\n*** cached 1M entries %u flow(s) (MPLS label = 1M -> 1)\n", n);
  for (i = 0; i < n; i++) {
    cache_enable(pkt[i]);
  }
  flow_benchmark(pkt, n, 1);
  print_flowcache_stats();
  for (i = 0; i < n; i++) {
    destroy_packet(pkt[i]);
  }
}

void
test_MPLS_1M_entries_flow_benchmark(void) {
  MPLS_1M_entries_n_flow_benchmark(1);
}

void
test_MPLS_1M_entries_1000_flow_benchmark(void) {
  MPLS_1M_entries_n_flow_benchmark(1000);
}

void
test_MPLS_1M_entries_1M_flow_benchmark(void) {
  MPLS_1M_entries_n_flow_benchmark(1000 * 1000);
}

#define IPADDR(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))
void
IPV4_SRC_65536_entries_flow_benchmark(size_t n) {
  struct lagopus_packet *pkt[n];
  size_t i;

  printf("\n*** 65536 entries %u flow(s) "
         "(IPV4_SRC 192.168.0.0-192.168.255.255)\n", n);
  ipsrc_flow_add(IPADDR(192,168,0,0), IPADDR(192,168,255,255), 32);
  for (i = 0; i < n; i++) {
    pkt[i] = build_ip_packet(1, IPADDR(192,168,1,i), IPADDR(192,168,1,2), 0);
  }
  flow_benchmark(pkt, n, 1);
  printf("\n*** cached 65536 entries %u flow(s) "
         "(IPV4_SRC 192.168.0.0-192.168.255.255)\n", n);
  for (i = 0; i < n; i++) {
    cache_enable(pkt[i]);
  }
  flow_benchmark(pkt, n, 1);
  print_flowcache_stats();
  for (i = 0; i < n; i++) {
    destroy_packet(pkt[i]);
  }
}

void
test_IPV4_SRC_65536_entries_1_flow_benchmark(void) {
  IPV4_SRC_65536_entries_flow_benchmark(1);
}

void
test_IPV4_SRC_65536_entries_256_flow_benchmark(void) {
  IPV4_SRC_65536_entries_flow_benchmark(256);
}

void
IPV4_DST_65536_entries_flow_benchmark(size_t n) {
  struct lagopus_packet *pkt[n];
  size_t i;

  printf("\n*** 65536 entries %u flow(s) "
         "(IPV4_DST 192.168.0.0-192.168.255.255)\n", n);
  ipdst_flow_add(IPADDR(192,168,0,0), IPADDR(192,168,255,255), 32);
  for (i = 0; i < n; i++) {
    pkt[i] = build_ip_packet(1, IPADDR(192,168,1,i), IPADDR(192,168,1,2), 0);
  }
  flow_benchmark(pkt, n, 1);
  printf("\n*** cached 65536 entries %u flow(S) "
         "(IPV4_DST 192.168.0.0-192.168.255.255)\n", n);
  for (i = 0; i < n; i++) {
    cache_enable(pkt[i]);
  }
  flow_benchmark(pkt, n, 1);
  print_flowcache_stats();
  for (i = 0; i < n; i++) {
    destroy_packet(pkt[i]);
  }
}

void
test_IPV4_DST_65536_entries_1_flow_benchmark(void) {
  IPV4_DST_65536_entries_flow_benchmark(1);
}

void
test_IPV4_DST_65536_entries_256_flow_benchmark(void) {
  IPV4_DST_65536_entries_flow_benchmark(256);
}
