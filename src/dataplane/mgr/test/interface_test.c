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

#include <sys/queue.h>
#include "unity.h"

#include "openflow.h"

#include "lagopus_apis.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"
#include "interface.c"

#include "lagopus/dp_apis.h"

#ifdef HAVE_DPDK
#include "../dpdk/build/include/rte_config.h"
#include "../dpdk/lib/librte_eal/common/include/rte_lcore.h"
#endif /* HYBRID */

#ifdef HYBRID
#include "lagopus/mactable.h"
#include "mactable.c"
#include "rib_notifier.c"
#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#endif /* HYBRID */

void
setUp(void) {
  dp_api_init();
}

void
tearDown(void) {
  dp_api_fini();
}

void
test_dp_interface_free(void) {
  struct interface *ifp;
  lagopus_result_t rv;

  ifp = dp_interface_alloc();
  TEST_ASSERT_NOT_NULL(ifp);
  rv = dp_interface_free(ifp);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_free(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
}

void
test_interface_create(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_ALREADY_EXISTS);
  rv = dp_interface_create(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
}

void
test_interface_destroy(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_destroy("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_destroy("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_destroy(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
}

void
test_interface_info_set(void) {
  datastore_interface_info_t info;
  lagopus_result_t rv;
#ifdef HYBRID
  lagopus_thread_t *thdptr;
  dp_tapio_thread_init(NULL, NULL, NULL, &thdptr);
#endif /* HYBRID */

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  memset(&info, 0, sizeof(info));
  info.type = DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY;
  info.eth_dpdk_phy.device = strdup("ffff:ff:ff.f");
  info.eth_dpdk_phy.ip_addr = strdup("192.168.0.1");
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK;
  info.eth_rawsock.device = strdup("lo00");
  info.eth_rawsock.ip_addr = strdup("192.168.0.1");
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_POSIX_API_ERROR);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = DATASTORE_INTERFACE_TYPE_VXLAN;
  info.vxlan.dst_addr = strdup("192.168.0.1");
  info.vxlan.src_addr = strdup("192.168.0.1");
  info.vxlan.ip_addr = strdup("192.168.0.1");
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = DATASTORE_INTERFACE_TYPE_UNKNOWN;
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = 0xffff;
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);

  rv = dp_interface_destroy("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = 0xffff;
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);

#ifdef HYBRID
  dp_tapio_thread_fini();
#endif /* HYBRID */
}

void
test_dp_interface_start_stop(void) {
  lagopus_result_t rv;
#ifdef HYBRID
  struct interface *ifp;
  lagopus_thread_t *thdptr;

  ifp = dp_interface_alloc();
  dp_tapio_thread_init(NULL, NULL, NULL, &thdptr);
  rv = dp_tap_interface_create("test", ifp);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
#endif /* HYBRID */

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_start("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_start("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stop(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
  rv = dp_interface_stop("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_stop("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

#ifdef HYBRID
  rv = dp_tap_interface_destroy("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_interface_free(ifp);
  dp_tapio_thread_fini();
#endif /* HYBRID */
}

void
test_dp_interface_stats(void) {
  datastore_interface_stats_t stats;
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stats_get("test", &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stats_get("test2", &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_stats_clear("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stats_clear("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
}

#ifdef HYBRID
static struct lagopus_packet *
create_l2pkt1 (struct port *port) {
  /*
   * create packet (A -> B)
   * host A(192.168.1.1[01:01]) , host B(192.168.1.2[01:02])
   * port 1([00:01]) , port 2([00:02])
   * host A - port 1, host B - port 2
   */
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* ether dest */
  OS_MTOD(m, uint8_t *)[0] = 0x00;
  OS_MTOD(m, uint8_t *)[1] = 0x00;
  OS_MTOD(m, uint8_t *)[2] = 0x00;
  OS_MTOD(m, uint8_t *)[3] = 0x00;
  OS_MTOD(m, uint8_t *)[4] = 0x01;
  OS_MTOD(m, uint8_t *)[5] = 0x02;
  /* ether src */
  OS_MTOD(m, uint8_t *)[6] = 0x00;
  OS_MTOD(m, uint8_t *)[7] = 0x00;
  OS_MTOD(m, uint8_t *)[8] = 0x00;
  OS_MTOD(m, uint8_t *)[9] = 0x00;
  OS_MTOD(m, uint8_t *)[10] = 0x01;
  OS_MTOD(m, uint8_t *)[11] = 0x01;

  /* in port number */
  port->ofp_port.port_no = 1;

  /* init lagopus packet */
  lagopus_packet_init(pkt, NULL, port);

  return pkt;
}

static struct lagopus_packet *
create_l2pkt2 (struct port *port) {
  /*
   * create packet (B -> A)
   * host A(192.168.1.1[01:01]) , host B(192.168.1.2[01:02])
   * port 1([00:01]) , port 2([00:02])
   * host A - port 1, host B - port 2
   */
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* ether dest */
  OS_MTOD(m, uint8_t *)[0] = 0x00;
  OS_MTOD(m, uint8_t *)[1] = 0x00;
  OS_MTOD(m, uint8_t *)[2] = 0x00;
  OS_MTOD(m, uint8_t *)[3] = 0x00;
  OS_MTOD(m, uint8_t *)[4] = 0x01;
  OS_MTOD(m, uint8_t *)[5] = 0x01;
  /* ether src */
  OS_MTOD(m, uint8_t *)[6] = 0x00;
  OS_MTOD(m, uint8_t *)[7] = 0x00;
  OS_MTOD(m, uint8_t *)[8] = 0x00;
  OS_MTOD(m, uint8_t *)[9] = 0x00;
  OS_MTOD(m, uint8_t *)[10] = 0x01;
  OS_MTOD(m, uint8_t *)[11] = 0x02;

  /* in port number */
  port->ofp_port.port_no = 2;

  /* init lagopus packet */
  lagopus_packet_init(pkt, NULL, port);

  return pkt;
}
#endif

void
test_interface_l2_switching(void) {
#if defined HYBRID && !defined PIPELINER
  char bridge_name[] = "br0";
  static uint32_t portid = 100;
  struct interface *ifp1, *ifp2;
  struct lagopus_packet *pkt;
  struct port *port;
  struct local_data *local;
  unsigned lcore_id = 0;
  lagopus_thread_t *thdptr;
  lagopus_result_t rv;
  struct mactable *mactable;
  uint64_t inteth;
  struct timespec now = get_current_time();
  uint8_t hostB_mac[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x02};

  /* preparation */
#ifdef HAVE_DPDK
  RTE_PER_LCORE(_lcore_id) = lcore_id;
#endif /* HAVE_DPDK */

  /*** preparation ***/
  /* create interface */
  /* port 1 */
  ifp1 = dp_interface_alloc();
  ifp1->hw_addr[0] = 0x00;
  ifp1->hw_addr[1] = 0x00;
  ifp1->hw_addr[2] = 0x00;
  ifp1->hw_addr[3] = 0x00;
  ifp1->hw_addr[4] = 0x00;
  ifp1->hw_addr[5] = 0x01;
  /* port 2 */
  ifp2 = dp_interface_alloc();
  ifp2->hw_addr[0] = 0x00;
  ifp2->hw_addr[1] = 0x00;
  ifp2->hw_addr[2] = 0x00;
  ifp2->hw_addr[3] = 0x00;
  ifp2->hw_addr[4] = 0x00;
  ifp2->hw_addr[5] = 0x02;

  /* create bridge and port */
  port = port_alloc();
  port->bridge = bridge_alloc(bridge_name);

  /*
   * TEST 1
   * host A -> host B
   * => flooding
   */
  pkt = create_l2pkt1(port);
  rv = interface_l2_switching(pkt, ifp1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(pkt->output_port, OFPP_ALL);

  /*
   * TEST 2
   * host A -> host B
   * => output port 2 (mactable)
   */
  mactable = &port->bridge->mactable;
  inteth = array_to_uint64(hostB_mac);
  add_entry_write_table(mactable, &mactable->hashmap[0],
                        inteth, 2, MACTABLE_SETTYPE_DYNAMIC, now);

  pkt = create_l2pkt1(port);
  rv = interface_l2_switching(pkt, ifp1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(pkt->output_port, 2);

  /*
   * TEST 3
   * host B -> host A
   * => output port 1 (local cache)
   */
  pkt = create_l2pkt2(port);
  rv = interface_l2_switching(pkt, ifp2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(pkt->output_port, 1);

  /* clean up */
  bridge_free(port->bridge);
  port_free(port);
  dp_interface_free(ifp1);
  dp_interface_free(ifp2);
#else /* HYBRID && !PIPELINER */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID && !PIPELINER */
}

#ifdef HYBRID
static struct lagopus_packet *
create_pkt1 (struct port *port) {
  /* create packet (A -> B) */

  /* host A(192.168.1.1[01:01]) , host B(192.168.2.2[02:02]) */
  /* port 1(192.168.1.101[00:01]) , port 2(192.168.2.102[00:02])*/
  /* host A - port 1, host B - port 2 */
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  struct in_addr src;
  struct in_addr dst;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* ether dest */
  OS_MTOD(m, uint8_t *)[0] = 0x00;
  OS_MTOD(m, uint8_t *)[1] = 0x00;
  OS_MTOD(m, uint8_t *)[2] = 0x00;
  OS_MTOD(m, uint8_t *)[3] = 0x00;
  OS_MTOD(m, uint8_t *)[4] = 0x00;
  OS_MTOD(m, uint8_t *)[5] = 0x01;
  /* ether src */
  OS_MTOD(m, uint8_t *)[6] = 0x00;
  OS_MTOD(m, uint8_t *)[7] = 0x00;
  OS_MTOD(m, uint8_t *)[8] = 0x00;
  OS_MTOD(m, uint8_t *)[9] = 0x00;
  OS_MTOD(m, uint8_t *)[10] = 0x01;
  OS_MTOD(m, uint8_t *)[11] = 0x01;
  /* ether type */
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  /* in port number */
  port->ofp_port.port_no = 1;

  /* init lagopus packet */
  lagopus_packet_init(pkt, NULL, port);

  /* ttl */
  IPV4_TTL(pkt->ipv4) = 64;

  /* ip src */
  inet_aton("192.168.1.1", &src);
  IPV4_SRC(pkt->ipv4) = src.s_addr;
  /* ip dest */
  inet_aton("192.168.2.2", &dst);
  IPV4_DST(pkt->ipv4) = dst.s_addr;

  return pkt;
}

static struct lagopus_packet *
create_pkt2 (struct port *port) {
  /* create packet(B -> A) */

  /* host A(192.168.1.1[01:01]) , host B(192.168.2.2[02:02]) */
  /* port 1(192.168.1.101[00:01]) , port 2(192.168.2.102[00:02])*/
  /* host A - port 1, host B - port 2 */
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  struct in_addr src;
  struct in_addr dst;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* ether dest */
  OS_MTOD(m, uint8_t *)[0] = 0x00;
  OS_MTOD(m, uint8_t *)[1] = 0x00;
  OS_MTOD(m, uint8_t *)[2] = 0x00;
  OS_MTOD(m, uint8_t *)[3] = 0x00;
  OS_MTOD(m, uint8_t *)[4] = 0x00;
  OS_MTOD(m, uint8_t *)[5] = 0x02;
  /* ether src */
  OS_MTOD(m, uint8_t *)[6] = 0x00;
  OS_MTOD(m, uint8_t *)[7] = 0x00;
  OS_MTOD(m, uint8_t *)[8] = 0x00;
  OS_MTOD(m, uint8_t *)[9] = 0x00;
  OS_MTOD(m, uint8_t *)[10] = 0x02;
  OS_MTOD(m, uint8_t *)[11] = 0x02;
  /* ether type */
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  /* in port number */
  port->ofp_port.port_no = 2;

  /* init lagopus packet */
  lagopus_packet_init(pkt, NULL, port);

  /* ttl */
  IPV4_TTL(pkt->ipv4) = 64;

  /* ip src */
  inet_aton("192.168.2.2", &src);
  IPV4_SRC(pkt->ipv4) = src.s_addr;
  /* ip dest */
  inet_aton("192.168.1.1", &dst);
  IPV4_DST(pkt->ipv4) = dst.s_addr;

  return pkt;
}
#endif /* HYBRID */

void
test_interface_l3_routing(void) {
#if defined HYBRID && !defined PIPELINER
  char bridge_name[] = "br0";
  static uint8_t scope = RT_SCOPE_LINK;
  static uint8_t port1_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  static uint8_t port2_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  static uint8_t hostA_mac[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
  static uint8_t hostB_mac[] = {0x00, 0x00, 0x00, 0x00, 0x02, 0x02};
  static uint64_t inteth = 0x100000000000;
  static int ifindex = 1, prefixlen = 24;
  struct interface *ifp1, *ifp2;
  struct lagopus_packet *pkt;
  struct port *port;
  struct bridge *br;
  struct local_data *local;
  struct in_addr dst, gate;
  lagopus_thread_t *thdptr;
  lagopus_result_t rv;
  struct in_addr self1, self2, broad;

#ifdef HAVE_DPDK
  unsigned lcore_id = 0;
  RTE_PER_LCORE(_lcore_id) = lcore_id;
#endif /* HAVE_DPDK */

  /*** preparation ***/
  /* create interface */
  inet_aton("192.168.1.101", &self1);
  inet_aton("192.168.2.102", &self2);
  inet_aton("255.255.255.0", &broad);
  /* port 1 */
  ifp1 = dp_interface_alloc();
  ifp1->addr_info.ip = self1;
  ifp1->addr_info.broad = broad;
  ifp1->addr_info.prefixlen = 24;
  /* port 2 */
  ifp2 = dp_interface_alloc();
  ifp2->addr_info.ip = self2;
  ifp2->addr_info.broad = broad;
  ifp2->addr_info.prefixlen = 24;

  /* create bridge and port */
  port = port_alloc();
  port->bridge = bridge_alloc(bridge_name);

  /*
   * TEST 1
   * learning mac address from host A.
   * no route to host.
   */
  pkt = create_pkt1(port);
  rv = interface_l3_routing(pkt, ifp1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(pkt->output_port, 0);

  /*
   * TEST 2
   * found route entry, but not found arp entry.
   * so, send packet to kernel.
   */
  /* add route entry to host B */
  dst.s_addr = inet_addr("192.168.2.0");
  rv = route_entry_add(&pkt->bridge->rib.ribs[0].route_table,
                       &dst, prefixlen, &gate,
                       200, scope, port2_mac);

  /* add route entry to host A */
  dst.s_addr = inet_addr("192.168.1.0");
  rv = route_entry_add(&pkt->bridge->rib.ribs[0].route_table,
                       &dst, prefixlen, &gate,
                       100, scope, port1_mac);

  pkt = create_pkt2(port);
  rv = interface_l3_routing(pkt, ifp2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(pkt->output_port, 0);
  /* here, packet isn't freed because we don't have a tap, so free it */
  lagopus_packet_free(pkt);

  /*
   * TEST 3
   * found route enetry and arp entry.
   * lookup output port.
   */
  /* add arp entry */
  gate.s_addr = inet_addr("192.168.1.1");
  arp_entry_update(&pkt->bridge->rib.ribs[0].arp_table,
                   100, &gate, hostA_mac);
  gate.s_addr = inet_addr("192.168.2.2");
  arp_entry_update(&pkt->bridge->rib.ribs[0].arp_table,
                   200, &gate, hostB_mac);

  pkt = create_pkt2(port);
  rv = interface_l3_routing(pkt, ifp2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(pkt->output_port, 1);

  /*** clean up ***/
  bridge_free(port->bridge);
  port_free(port);
  dp_interface_free(ifp1);
  dp_interface_free(ifp2);
#else /* HYBRID && !PIPELINER */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID && !PIPELINER */
}

void
test_dp_interface_ip_set_unset(void) {
#ifdef HYBRID
  struct in_addr ip, broad;
  uint8_t prefixlen = 32;
  lagopus_result_t rv;

  ip.s_addr = inet_addr("192.168.0.1");
  broad.s_addr = inet_addr("255.255.0.0");
  rv = dp_interface_create(":test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  rv = dp_interface_ip_set("test", AF_INET, &ip, &broad, prefixlen);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_ip_set("test2", AF_INET, &ip, &broad, prefixlen);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_ip_unset("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_ip_unset("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);

  rv = dp_interface_stats_clear(":test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_dp_interface_ip_get(void) {
#ifdef HYBRID
  struct interface *ifp;
  struct in_addr ip, broad;
  uint8_t prefixlen;
  lagopus_result_t rv;

  ifp = dp_interface_alloc();
  ifp->addr_info.ip.s_addr = inet_addr("192.168.0.1");
  ifp->addr_info.broad.s_addr = inet_addr("255.255.0.0");
  ifp->addr_info.prefixlen = 32;

  rv = dp_interface_ip_get(ifp, AF_INET, &ip, &broad, &prefixlen);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  dp_interface_free(ifp);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_dp_interface_send_packet_normal(void) {
#ifdef HYBRID
  struct interface *ifp;
  struct lagopus_packet *pkt;
  lagopus_result_t rv;

  ifp = dp_interface_alloc();
  pkt = alloc_lagopus_packet();

  rv = dp_interface_send_packet_normal(pkt, ifp, true);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  ifp->addr_info.set = true;
  rv = dp_interface_send_packet_normal(pkt, ifp, true);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  rv = dp_interface_send_packet_normal(pkt, ifp, false);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  dp_interface_free(ifp);
  lagopus_packet_free(pkt);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}
