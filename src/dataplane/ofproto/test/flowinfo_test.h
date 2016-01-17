#ifndef __SRC_DATAPLANE_OFPROTO_TEST_FLOWINFO_TEST_H__
#define __SRC_DATAPLANE_OFPROTO_TEST_FLOWINFO_TEST_H__

/*
 * The common macros for the flowinfo unittests.
 */

/*
 * Test flow numbers.
 * (extra: for the negative deletion tests only)
 */
#define TEST_FLOW_NUM		(3)
#define TEST_FLOW_EXTRA_NUM	(1)

/*
 * Extract the field type from a match.
 * XXX should be defined globally.
 */
#define _OXM_FIELD_TYPE(_field)	((_field) >> 1)

/*
 * Declare the common global data for a unittest.
 */
#define FLOWINFO_TEST_DECLARE_DATA\
  /* The root flowinfo. */\
  static struct flowinfo *flowinfo;\
  \
  /* The test flows. */\
  static struct flow *test_flow[TEST_FLOW_NUM + TEST_FLOW_EXTRA_NUM];\
   

/*
 * Flow operations.
 */

/* Add the prerequisite match of ARP. */
#define FLOW_ADD_ARP_PREREQUISITE(_fl)			\
  do {							\
    /* Ethernet type. */				\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHERTYPE_ARP);	\
  } while (0)

/* Add the prerequisite match of IPv4. */
#define FLOW_ADD_IPV4_PREREQUISITE(_fl)			\
  do {							\
    /* Ethernet type. */				\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHERTYPE_IP);	\
  } while (0)

/* Add the prerequisite match of IPv6. */
#define FLOW_ADD_IPV6_PREREQUISITE(_fl)			\
  do {							\
    /* Ethernet type. */				\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHERTYPE_IPV6);	\
  } while (0)

/* Add the prerequisite match of IPv4 TCP. */
#define FLOW_ADD_IPV4_TCP_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV4_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_TCP);	\
  } while (0)

/* Add the prerequisite match of IPv4 UDP. */
#define FLOW_ADD_IPV4_UDP_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV4_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_UDP);	\
  } while (0)

/* Add the prerequisite match of IPv4 SCTP. */
#define FLOW_ADD_IPV4_SCTP_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV4_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_SCTP);	\
  } while (0)

/* Add the prerequisite match of IPv4 ICMP. */
#define FLOW_ADD_IPV4_ICMP_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV4_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_ICMP);	\
  } while (0)

/* Add the prerequisite match of IPv6 TCP. */
#define FLOW_ADD_IPV6_TCP_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV6_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_TCP);	\
  } while (0)

/* Add the prerequisite match of IPv6 UDP. */
#define FLOW_ADD_IPV6_UDP_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV6_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_UDP);	\
  } while (0)

/* Add the prerequisite match of IPv6 SCTP. */
#define FLOW_ADD_IPV6_SCTP_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV6_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_SCTP);	\
  } while (0)

/* Add the prerequisite match of IPv6 ICMPv6. */
#define FLOW_ADD_IPV6_ICMPV6_PREREQUISITE(_fl)		\
  do {							\
    FLOW_ADD_IPV6_PREREQUISITE((_fl));			\
    /* IP protocol. */					\
    FLOW_ADD_IP_PROTO_MATCH((_fl), IPPROTO_ICMPV6);	\
  } while (0)

/* Add the prerequisite match of IPv6 ND NS. */
#define FLOW_ADD_IPV6_ND_NS_PREREQUISITE(_fl)			\
  do {								\
    FLOW_ADD_IPV6_ICMPV6_PREREQUISITE((_fl));			\
    /* ICMPv6 ND. */						\
    FLOW_ADD_ICMPV6_TYPE_MATCH((_fl), ND_NEIGHBOR_SOLICIT);	\
  } while (0)

/* Add the prerequisite match of IPv6 ND NA. */
#define FLOW_ADD_IPV6_ND_NA_PREREQUISITE(_fl)			\
  do {								\
    FLOW_ADD_IPV6_ICMPV6_PREREQUISITE((_fl));			\
    /* ICMPv6 NA. */						\
    FLOW_ADD_ICMPV6_TYPE_MATCH((_fl), ND_NEIGHBOR_ADVERT);	\
  } while (0)

/* Add the prerequisite match of MPLS. */
#define FLOW_ADD_MPLS_PREREQUISITE(_fl)			\
  do {							\
    /* Ethernet type. */				\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHER_TYPE_MPLS);	\
  } while (0)

/* Add the prerequisite match of MPLS multicast. */
#define FLOW_ADD_MMPLS_PREREQUISITE(_fl)			\
  do {								\
    /* Ethernet type. */					\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHER_TYPE_MPLS_MCAST);	\
  } while (0)

/* Add the prerequisite match of PBB. */
#define FLOW_ADD_PBB_PREREQUISITE(_fl)			\
  do {							\
    /* Ethernet type. */				\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHERTYPE_PBB);	\
  } while (0)

/* Add the prerequisite match of a physical port. */
#define FLOW_ADD_PHYPORT_PREREQUISITE(_fl, _iport)	\
  do {							\
    /* Ingress port. */					\
    FLOW_ADD_PORT_MATCH((_fl), (_iport));		\
  } while (0)

/* Add the prerequisite match of VLAN PCP. */
#define FLOW_ADD_VLAN_PCP_PREREQUISITE(_fl)		\
  do {							\
    /* VLAN tag existence. */				\
    FLOW_ADD_VLAN_VID_W_MATCH((_fl), OFPVID_PRESENT, OFPVID_PRESENT);	\
  } while (0)

/* Add the prerequisite match of MPLS. */
#define FLOW_ADD_MPLS_PREREQUISITE(_fl)			\
  do {							\
    /* Ethernet type. */				\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHER_TYPE_MPLS);	\
  } while (0)

/* Add the prerequisite match of MPLS multicast. */
#define FLOW_ADD_MMPLS_PREREQUISITE(_fl)			\
  do {								\
    /* Ethernet type. */					\
    FLOW_ADD_ETH_TYPE_MATCH((_fl), ETHER_TYPE_MPLS_MCAST);	\
  } while (0)


/*
 * Assertion tools.
 */

/* Assert the Ethernet type match of a flow. */
#define TEST_ASSERT_FLOW_MATCH_ETH_TYPE(_fl, _type)			\
  do {									\
    int _found = 0;							\
    struct match *_m;							\
    uint16_t _t;							\
    TAILQ_FOREACH(_m, &((_fl)->match_list), entry) {			\
      if (OFPXMT_OFB_ETH_TYPE == _OXM_FIELD_TYPE(_m->oxm_field)) {	\
        OS_MEMCPY(&_t, _m->oxm_value, sizeof(_t));			\
        TEST_ASSERT_EQUAL_INT((_type), OS_NTOHS(_t));			\
        _found = 1;							\
        break;								\
      }									\
    }									\
    TEST_ASSERT_TRUE(_found);						\
  } while (0)

/* Assert the IP protocol match of a flow. */
#define TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL(_fl, _proto)			\
  do {									\
    int __found = 0;							\
    struct match *__m;							\
    uint8_t __t;							\
    TAILQ_FOREACH(__m, &((_fl)->match_list), entry) {			\
      if (OFPXMT_OFB_IP_PROTO == _OXM_FIELD_TYPE(__m->oxm_field)) {	\
        OS_MEMCPY(&__t, __m->oxm_value, sizeof(__t));			\
        TEST_ASSERT_EQUAL_INT((_proto), __t);				\
        __found = 1;							\
        break;								\
      }									\
    }									\
    TEST_ASSERT_TRUE(__found);						\
  } while (0)

/* Assert the ICMPv6 type match of a flow. */
#define TEST_ASSERT_FLOW_MATCH_ICMPV6_TYPE(_fl, _type)			\
  do {									\
    int __found = 0;							\
    struct match *__m;							\
    uint8_t __t;							\
    TAILQ_FOREACH(__m, &((_fl)->match_list), entry) {			\
      if (OFPXMT_OFB_ICMPV6_TYPE == _OXM_FIELD_TYPE(__m->oxm_field)) {	\
        OS_MEMCPY(&__t, __m->oxm_value, sizeof(__t));			\
        TEST_ASSERT_EQUAL_INT((_type), __t);				\
        __found = 1;							\
        break;								\
      }									\
    }									\
    TEST_ASSERT_TRUE(__found);						\
  } while (0)

/* Assert the port match of a flow. */
#define TEST_ASSERT_FLOW_MATCH_PORT(_fl, _port)				\
  do {									\
    int __found = 0;							\
    struct match *__m;							\
    uint32_t __t;							\
    TAILQ_FOREACH(__m, &((_fl)->match_list), entry) {			\
      if (OFPXMT_OFB_IN_PORT == _OXM_FIELD_TYPE(__m->oxm_field)) {	\
        OS_MEMCPY(&__t, __m->oxm_value, sizeof(__t));			\
        TEST_ASSERT_EQUAL_INT((_port), OS_NTOHL(__t));			\
        __found = 1;							\
        break;								\
      }									\
    }									\
    TEST_ASSERT_TRUE(__found);						\
  } while (0)

/* Assert the prerequisite match of ARP. */
#define TEST_ASSERT_FLOW_MATCH_ARP_PREREQUISITE(_fl)		\
  do {								\
    TEST_ASSERT_FLOW_MATCH_ETH_TYPE((_fl), ETHERTYPE_ARP);	\
  } while (0)

/* Assert the prerequisite match of IPv4. */
#define TEST_ASSERT_FLOW_MATCH_IPV4_PREREQUISITE(_fl)		\
  do {								\
    TEST_ASSERT_FLOW_MATCH_ETH_TYPE((_fl), ETHERTYPE_IP);	\
  } while (0)

/* Assert the prerequisite match of IPv6. */
#define TEST_ASSERT_FLOW_MATCH_IPV6_PREREQUISITE(_fl)		\
  do {								\
    TEST_ASSERT_FLOW_MATCH_ETH_TYPE((_fl), ETHERTYPE_IPV6);	\
  } while (0)

/* Assert the prerequisite match of IPv4 TCP. */
#define TEST_ASSERT_FLOW_MATCH_IPV4_TCP_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV4_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_TCP);	\
  } while (0)

/* Assert the prerequisite match of IPv4 UDP. */
#define TEST_ASSERT_FLOW_MATCH_IPV4_UDP_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV4_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_UDP);	\
  } while (0)

/* Assert the prerequisite match of IPv4 SCTP. */
#define TEST_ASSERT_FLOW_MATCH_IPV4_SCTP_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV4_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_SCTP);	\
  } while (0)

/* Assert the prerequisite match of IPv4 ICMP. */
#define TEST_ASSERT_FLOW_MATCH_IPV4_ICMP_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV4_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_ICMP);	\
  } while (0)

/* Assert the prerequisite match of IPv6 TCP. */
#define TEST_ASSERT_FLOW_MATCH_IPV6_TCP_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV6_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_TCP);	\
  } while (0)

/* Assert the prerequisite match of IPv6 UDP. */
#define TEST_ASSERT_FLOW_MATCH_IPV6_UDP_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV6_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_UDP);	\
  } while (0)

/* Assert the prerequisite match of IPv6 SCTP. */
#define TEST_ASSERT_FLOW_MATCH_IPV6_SCTP_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV6_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_SCTP);	\
  } while (0)

/* Assert the prerequisite match of IPv6 ICMPv6. */
#define TEST_ASSERT_FLOW_MATCH_IPV6_ICMPV6_PREREQUISITE(_fl)	\
  do {								\
    TEST_ASSERT_FLOW_MATCH_IPV6_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_IP_PROTOCOL((_fl), IPPROTO_ICMPV6);	\
  } while (0)

/* Assert the prerequisite match of IPv6 ND NS. */
#define TEST_ASSERT_FLOW_MATCH_IPV6_ND_NS_PREREQUISITE(_fl)		\
  do {									\
    TEST_ASSERT_FLOW_MATCH_IPV6_ICMPV6_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_ICMPV6_TYPE((_fl), ND_NEIGHBOR_SOLICIT);	\
  } while (0)

/* Assert the prerequisite match of IPv6 ND NA. */
#define TEST_ASSERT_FLOW_MATCH_IPV6_ND_NA_PREREQUISITE(_fl)		\
  do {									\
    TEST_ASSERT_FLOW_MATCH_IPV6_ICMPV6_PREREQUISITE((_fl));		\
    TEST_ASSERT_FLOW_MATCH_ICMPV6_TYPE((_fl), ND_NEIGHBOR_ADVERT);	\
  } while (0)

/* Assert the prerequisite match of MPLS. */
#define TEST_ASSERT_FLOW_MATCH_MPLS_PREREQUISITE(_fl)		\
  do {								\
    TEST_ASSERT_FLOW_MATCH_ETH_TYPE((_fl), ETHER_TYPE_MPLS);	\
  } while (0)

/* Assert the prerequisite match of MPLS multicast. */
#define TEST_ASSERT_FLOW_MATCH_MMPLS_PREREQUISITE(_fl)			\
  do {									\
    TEST_ASSERT_FLOW_MATCH_ETH_TYPE((_fl), ETHER_TYPE_MPLS_MCAST);	\
  } while (0)

/* Assert the prerequisite match of PBB. */
#define TEST_ASSERT_FLOW_MATCH_PBB_PREREQUISITE(_fl)		\
  do {								\
    TEST_ASSERT_FLOW_MATCH_ETH_TYPE((_fl), ETHERTYPE_PBB);	\
  } while (0)

/* Assert the prerequisite match of a physical port. */
#define TEST_ASSERT_FLOW_MATCH_PHYPORT_PREREQUISITE(_fl, _iport)	\
  do {									\
    TEST_ASSERT_FLOW_MATCH_PORT((_fl), (_iport));			\
  } while (0)

/* Assert the prerequisite match of VLAN PCP. */
#define TEST_ASSERT_FLOW_MATCH_VLAN_PCP_PREREQUISITE(_fl)		\
  do {									\
    int __found = 0;							\
    struct match *__m;							\
    uint16_t __t;							\
    TAILQ_FOREACH(__m, &((_fl)->match_list), entry) {			\
      if (OFPXMT_OFB_VLAN_VID == _OXM_FIELD_TYPE(__m->oxm_field)) {	\
        OS_MEMCPY(&__t, __m->oxm_value, sizeof(__t));			\
        TEST_ASSERT_TRUE(0 != (OFPVID_PRESENT & OS_NTOHS(__t)));	\
        __found = 1;							\
        break;								\
      }									\
    }									\
    TEST_ASSERT_TRUE(__found);						\
  } while (0)


/*
 * Test scenario helpers.
 * XXX maybe these should go to the unittest library.
 */

/* Positively assert flow existence. */
#define _TEST_ASSERT_FLOWINFO_FINDFLOW_OK(_fl, _bi, _ei, _msg)		\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_FIND_OK((_fl), test_flow[_s], (_msg));	\
  } while (0)

/* Negatively assert flow existence. */
#define _TEST_ASSERT_FLOWINFO_FINDFLOW_NG(_fl, _bi, _ei, _msg)		\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_FIND_NG((_fl), test_flow[_s], (_msg));	\
  } while (0)

/* Assert flow existence. */
#define TEST_ASSERT_FLOWINFO_FINDFLOW(_fl, _expected, _msg)		\
  do {									\
    char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    if (_expected) {							\
      snprintf(__buf, sizeof(__buf), "%s, finding flow(ok)", (_msg));   \
      _TEST_ASSERT_FLOWINFO_FINDFLOW_OK((_fl), 0, TEST_FLOW_NUM, __buf); \
    } else {								\
      snprintf(__buf, sizeof(__buf), "%s, finding flow(ng)", (_msg));   \
      _TEST_ASSERT_FLOWINFO_FINDFLOW_NG((_fl), 0, TEST_FLOW_NUM, __buf); \
    }                                                                   \
    snprintf(__buf, sizeof(__buf), "%s, finding flow(extre)", (_msg));  \
    _TEST_ASSERT_FLOWINFO_FINDFLOW_NG((_fl),				\
                                      TEST_FLOW_NUM, TEST_FLOW_EXTRA_NUM, __buf); \
  } while (0)

/* Make the assertion message string for each step. */
#define TEST_SCENARIO_MSG_STEP(_buf, _step)		\
  do {							\
    snprintf((_buf), sizeof(_buf), "Step %d", (_step));	\
  } while (0)


/*
 * Test scenarios.
 *
 * These macros require the following user-defined macros:
 * - TEST_ASSERT_FLOWINFO_ADDFLOW_OK()
 * - TEST_ASSERT_FLOWINFO_DELFLOW_OK()
 * - TEST_ASSERT_FLOWINFO_DELFLOW_NG()
 *
 * TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN() is optional.  This is the
 * same assertion as TEST_ASSERT_FLOWINFO_DELFLOW_NG(); in addition,
 * it also tests the cleanliness of the DUT state.
 *
 * If not given, it defaults to TEST_ASSERT_FLOWINFO_DELFLOW_NG().
 */
#ifndef TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN
#define TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN(_fl, _bi, _ei, _flnum, _msg) \
  TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, _flnum, _msg)
#endif /* TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN */

/*
 * The test scenario for the sideeffect-free DUT.  (ie the DUT state
 * reverts after the flow removal)
 *
 * Steps:
 * 0. Assert no flows found in a flowinfo.
 * 1. Add flows.
 * 2. Doubly add flows.
 * 3. Delete flows.
 * 4. Doubly delete flows.
 * 5. Delete non-existent flows.
 */
#define TEST_SCENARIO_FLOWINFO_SEF(_fi)					\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    /* Step 0. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 0);					\
    TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, _buf);			\
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
    \
    /* Step 1. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 1);					\
    TEST_ASSERT_FLOWINFO_ADDFLOW_OK((_fi), 0, TEST_FLOW_NUM, TEST_FLOW_NUM, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), true, _buf);			\
    \
    /* Step 2. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 2);					\
    TEST_ASSERT_FLOWINFO_ADDFLOW_OK((_fi), 0, TEST_FLOW_NUM, 2 * TEST_FLOW_NUM, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), true, _buf);			\
    \
    /* Step 3. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 3);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_OK((_fi), 0, TEST_FLOW_NUM, TEST_FLOW_NUM, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), true, _buf);			\
    \
    /* Step 4. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 4);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_OK((_fi), 0, TEST_FLOW_NUM, 0, _buf);	\
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
    \
    /* Step 5. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 5);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_NG((_fi), 0, TEST_FLOW_NUM, 0, _buf);	\
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
    \
    /* Step 6. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 6);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_NG((_fi), TEST_FLOW_NUM, TEST_FLOW_EXTRA_NUM, 0, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
  } while (0)

/*
 * The test scenario for the sideeffect-prone DUT.  (ie the DUT state
 * changes after the flow removal)
 *
 * Steps:
 * 0. Assert no flows found in a flowinfo.
 * 1. Delete non-existent flows in the clean DUT.
 * 2. Add flows.
 * 3. Doubly add flows.
 * 4. Delete flows.
 * 5. Doubly delete flows.
 * 6. Delete non-existent flows.
 * 7. Add flows again.
 * 8. Delete flows again.
 */
#define TEST_SCENARIO_FLOWINFO_SEP(_fi)					\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    /* Step 0. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 0);					\
    TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, _buf);			\
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
    \
    /* Step 1. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 1);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN((_fi), TEST_FLOW_NUM, TEST_FLOW_EXTRA_NUM, 0, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
    \
    /* Step 2. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 2);					\
    TEST_ASSERT_FLOWINFO_ADDFLOW_OK((_fi), 0, TEST_FLOW_NUM, TEST_FLOW_NUM, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), true, _buf);			\
    \
    /* Step 3. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 3);					\
    TEST_ASSERT_FLOWINFO_ADDFLOW_OK((_fi), 0, TEST_FLOW_NUM, 2 * TEST_FLOW_NUM, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), true, _buf);			\
    \
    /* Step 4. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 4);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_OK((_fi), 0, TEST_FLOW_NUM, TEST_FLOW_NUM, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), true, _buf);			\
    \
    /* Step 5. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 5);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_OK((_fi), 0, TEST_FLOW_NUM, 0, _buf);	\
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
    \
    /* Step 6. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 6);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_NG((_fi), 0, TEST_FLOW_NUM, 0, _buf);	\
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
    \
    /* Step 7. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 7);					\
    TEST_ASSERT_FLOWINFO_ADDFLOW_OK((_fi), 0, TEST_FLOW_NUM, TEST_FLOW_NUM, _buf); \
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), true, _buf);			\
    \
    /* Step 8. */							\
    TEST_SCENARIO_MSG_STEP(_buf, 8);					\
    TEST_ASSERT_FLOWINFO_DELFLOW_OK((_fi), 0, TEST_FLOW_NUM, 0, _buf);	\
    TEST_ASSERT_FLOWINFO_FINDFLOW((_fi), false, _buf);			\
  } while (0)

#endif /* __SRC_DATAPLANE_OFPROTO_TEST_FLOWINFO_TEST_H__ */
