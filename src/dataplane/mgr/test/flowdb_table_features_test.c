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

#include "unity.h"

#include "lagopus/flowdb.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/datastore/bridge.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


/*
 * Feature checklist entry.
 */
struct feature_checklist {
  uint64_t	fc_key;
  int		fc_checked;
};

/* Init a feature checklist. */
#define INIT_FEATURE_CHECKLIST(_fc, _keys)		\
  do {							\
    size_t _s;						\
    memset((_fc), 0, sizeof(_fc));			\
    for (_s = 0; _s < ARRAY_LEN(_fc); _s++)		\
      (_fc)[_s].fc_key = (uint64_t)((_keys)[_s]);	\
  } while (0)

/* Check a feature checklist entry. */
#define CHECK_FEATURE_CHECKLIST(_fc, _key)				\
  do {									\
    size_t _s;								\
    int _found;								\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "key %lu unknown", (uint64_t)_key);	\
    _found = 0;								\
    for (_s = 0; _s < ARRAY_LEN(_fc); _s++)				\
      if ((uint64_t)(_key) == (_fc)[_s].fc_key) {			\
        (_fc)[_s].fc_checked = 1;					\
        _found = 1;							\
      }									\
    TEST_ASSERT_TRUE_MESSAGE(_found, _buf);				\
  } while (0)

/* Assert a feature checklist. */
#define TEST_ASSERT_FEATURE_CHECKLIST(_fc)				\
  do {									\
    size_t _s;								\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    for (_s = 0; _s < ARRAY_LEN(_fc); _s++) {				\
      snprintf(_buf, sizeof(_buf), "key %lu not found", (_fc)[_s].fc_key); \
      TEST_ASSERT_TRUE_MESSAGE((_fc)[_s].fc_checked, _buf);		\
    }									\
  } while (0)

/* The scenario for a feature check. */
#define TEST_SCENARIO_FEATURE_CHECK(_pt, _it, _pm, _im, _keys)	\
  do {								\
    struct feature_checklist _fc[ARRAY_LEN(_keys) - 1];		\
    const struct table_features *_f;				\
    const struct table_property *_p;				\
    const _it *_i;						\
    \
    INIT_FEATURE_CHECKLIST(_fc, (_keys));			\
    \
    TAILQ_FOREACH(_f, &features_list, entry) {			\
      TAILQ_FOREACH(_p, &_f->table_property_list, entry) {	\
        if ((_pt) != _p->ofp.type)				\
          continue;						\
        TAILQ_FOREACH(_i, &_p->_pm, entry) {			\
          CHECK_FEATURE_CHECKLIST(_fc, _i->_im);		\
        }							\
      }								\
    }								\
    \
    TEST_ASSERT_FEATURE_CHECKLIST(_fc);				\
  } while (0)

/* The scenario for an empty feature. */
#define TEST_SCENARIO_FEATURE_EMPTY(_pt, _it, _pm, _im)		\
  do {								\
    const struct table_features *_f;				\
    const struct table_property *_p;				\
    const _it *_i;						\
    int _count;							\
    \
    _count = 0;							\
    \
    TAILQ_FOREACH(_f, &features_list, entry) {			\
      TAILQ_FOREACH(_p, &_f->table_property_list, entry) {	\
        if ((_pt) != _p->ofp.type)				\
          continue;						\
        TAILQ_FOREACH(_i, &_p->_pm, entry) {			\
          _count++;						\
        }							\
      }								\
    }								\
    \
    TEST_ASSERT_EQUAL_INT(_count, 0);				\
  } while (0)

/*
 * Global variables.
 */
static struct bridge *bridge;
static struct flowdb *flowdb;
static struct table_features_list features_list;
static const char bridge_name[] = "br0";
static const uint64_t dpid = 12345678;


void
setUp(void) {
  datastore_bridge_info_t info;
  struct ofp_error err;

  TEST_ASSERT_NULL(bridge);
  TEST_ASSERT_NULL(flowdb);

  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dp_bridge_create(bridge_name, &info));
  TEST_ASSERT_NOT_NULL(bridge = dp_bridge_lookup(bridge_name));
  TEST_ASSERT_NOT_NULL(flowdb = bridge->flowdb);

  TAILQ_INIT(&features_list);
  memset(&err, 0, sizeof(err));
  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == flowdb_get_table_features(flowdb,
                   &features_list, &err));
}

void
tearDown(void) {
  struct table_features *f;
  struct table_property *p;
  struct next_table_id *n;
  struct oxm_id *o;
  struct experimenter_data *e;

  TEST_ASSERT_NOT_NULL(bridge);
  TEST_ASSERT_NOT_NULL(flowdb);

  /*
   * Inline expansion from outside dpmgr.
   *
   * Stolen form: src/agent/ofp_table_features_handler.c
   */
  while ((f = TAILQ_FIRST(&features_list)) != NULL) {
    while ((p = TAILQ_FIRST(&f->table_property_list)) != NULL) {
      instruction_list_entry_free(&p->instruction_list);

      /* next_table_id_list_entry_free(&p->next_table_id_list); */
      while ((n = TAILQ_FIRST(&p->next_table_id_list)) != NULL) {
        TAILQ_REMOVE(&p->next_table_id_list, n, entry);
        free(n);
      }

      action_list_entry_free(&p->action_list);

      /* oxm_id_list_entry_free(&p->oxm_id_list); */
      while ((o = TAILQ_FIRST(&p->oxm_id_list)) != NULL) {
        TAILQ_REMOVE(&p->oxm_id_list, o, entry);
        free(o);
      }

      /* experimenter_data_list_entry_free(&p->experimenter_data_list); */
      while ((e = TAILQ_FIRST(&p->experimenter_data_list)) != NULL) {
        TAILQ_REMOVE(&p->experimenter_data_list, e, entry);
        free(e);
      }

      TAILQ_REMOVE(&f->table_property_list, p, entry);
      free(p);
    }
    TAILQ_REMOVE(&features_list, f, entry);
    free(f);
  }
  TAILQ_INIT(&features_list);

  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dp_bridge_destroy(bridge_name));

  flowdb = NULL;
  bridge = NULL;
}


/*
 * Instructions.
 */
static const uint16_t inst_type[] = {
  OFPIT_GOTO_TABLE,
  OFPIT_WRITE_METADATA,
  OFPIT_WRITE_ACTIONS,
  OFPIT_APPLY_ACTIONS,
  OFPIT_CLEAR_ACTIONS,
  OFPIT_METER,
  0
};

void
test_flowdb_table_features_instructions(void) {
  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_INSTRUCTIONS, struct instruction,
                              instruction_list, ofpit.type, inst_type);
}

/*
 * Next table.
 */
void
test_flowdb_table_features_next_table(void) {
  uint8_t table_ids[254];
  uint8_t i, n;

  /* table_id[0..252] = 1..253; */
  for (i = 0, n = 1; n < OFPTT_MAX; i++, n++) {
    table_ids[i] = n;
  }
  table_ids[i] = 0;

  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_NEXT_TABLES, struct next_table_id,
                              next_table_id_list, ofp.id, table_ids);
}

/*
 * Write/apply actions.
 *
 * XXX a writable action is applicable and vice versa in lagopus.
 */

static const int action_type[] = {
  OFPAT_OUTPUT,
  OFPAT_COPY_TTL_OUT,
  OFPAT_COPY_TTL_IN,
  OFPAT_SET_MPLS_TTL,
  OFPAT_DEC_MPLS_TTL,
  OFPAT_PUSH_VLAN,
  OFPAT_POP_VLAN,
  OFPAT_PUSH_MPLS,
  OFPAT_POP_MPLS,
  /*OFPAT_SET_QUEUE,*/
  OFPAT_GROUP,
  OFPAT_SET_NW_TTL,
  OFPAT_DEC_NW_TTL,
  OFPAT_SET_FIELD,
  OFPAT_PUSH_PBB,
  OFPAT_POP_PBB,
  -1
};

void
test_flowdb_table_features_write_actions(void) {
  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_WRITE_ACTIONS, struct action, action_list,
                              ofpat.type, action_type);
}

void
test_flowdb_table_features_apply_actions(void) {
  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_APPLY_ACTIONS, struct action, action_list,
                              ofpat.type, action_type);
}

/*
 * Match and wildcard fields.
 *
 * XXX a matchable field can always be wildcarded in lagopus.
 */
static const uint32_t oxm_ids[] = {
  OXM_OF_IN_PORT,
  OXM_OF_IN_PHY_PORT,
  OXM_OF_METADATA,
  OXM_OF_METADATA_W,
  OXM_OF_ETH_DST,
  OXM_OF_ETH_DST_W,
  OXM_OF_ETH_SRC,
  OXM_OF_ETH_SRC_W,
  OXM_OF_ETH_TYPE,
  OXM_OF_VLAN_VID,
  OXM_OF_VLAN_VID_W,
  OXM_OF_VLAN_PCP,
  OXM_OF_IP_DSCP,
  OXM_OF_IP_ECN,
  OXM_OF_IP_PROTO,
  OXM_OF_IPV4_SRC,
  OXM_OF_IPV4_SRC_W,
  OXM_OF_IPV4_DST,
  OXM_OF_IPV4_DST_W,
  OXM_OF_TCP_SRC,
  OXM_OF_TCP_DST,
  OXM_OF_UDP_SRC,
  OXM_OF_UDP_DST,
  OXM_OF_SCTP_SRC,
  OXM_OF_SCTP_DST,
  OXM_OF_ICMPV4_TYPE,
  OXM_OF_ICMPV4_CODE,
  OXM_OF_ARP_OP,
  OXM_OF_ARP_SPA,
  OXM_OF_ARP_SPA_W,
  OXM_OF_ARP_TPA,
  OXM_OF_ARP_TPA_W,
  OXM_OF_ARP_SHA,
  OXM_OF_ARP_THA,
  OXM_OF_IPV6_SRC,
  OXM_OF_IPV6_SRC_W,
  OXM_OF_IPV6_DST,
  OXM_OF_IPV6_DST_W,
  OXM_OF_IPV6_FLABEL,
  OXM_OF_ICMPV6_TYPE,
  OXM_OF_ICMPV6_CODE,
  OXM_OF_IPV6_ND_TARGET,
  OXM_OF_IPV6_ND_SLL,
  OXM_OF_IPV6_ND_TLL,
  OXM_OF_MPLS_LABEL,
  OXM_OF_MPLS_TC,
  OXM_OF_MPLS_BOS,
  OXM_OF_PBB_ISID,
  OXM_OF_PBB_ISID_W,
  OXM_OF_TUNNEL_ID,
  OXM_OF_TUNNEL_ID_W,
  OXM_OF_IPV6_EXTHDR,
  OXM_OF_IPV6_EXTHDR_W,
  0
};

void
test_flowdb_table_features_matches(void) {
  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_MATCH, struct oxm_id, oxm_id_list, ofp.id,
                              oxm_ids);
}

void
test_flowdb_table_features_wildcards(void) {
  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_WILDCARDS, struct oxm_id, oxm_id_list,
                              ofp.id, oxm_ids);
}

/*
 * Settable fields.
 *
 * XXX a settable fields can both be written and applied in lagopus.
 */
static const uint32_t set_field_oxm_ids[] = {
  OXM_OF_IN_PORT,
  OXM_OF_METADATA,
  OXM_OF_ETH_DST,
  OXM_OF_ETH_SRC,
  OXM_OF_ETH_TYPE,
  OXM_OF_VLAN_VID,
  OXM_OF_VLAN_PCP,
  OXM_OF_IP_DSCP,
  OXM_OF_IP_ECN,
  OXM_OF_IP_PROTO,
  OXM_OF_IPV4_SRC,
  OXM_OF_IPV4_DST,
  OXM_OF_TCP_SRC,
  OXM_OF_TCP_DST,
  OXM_OF_UDP_SRC,
  OXM_OF_UDP_DST,
  OXM_OF_SCTP_SRC,
  OXM_OF_SCTP_DST,
  OXM_OF_ICMPV4_TYPE,
  OXM_OF_ICMPV4_CODE,
  OXM_OF_ARP_OP,
  OXM_OF_ARP_SPA,
  OXM_OF_ARP_TPA,
  OXM_OF_ARP_SHA,
  OXM_OF_ARP_THA,
  OXM_OF_IPV6_SRC,
  OXM_OF_IPV6_DST,
  OXM_OF_IPV6_FLABEL,
  OXM_OF_ICMPV6_TYPE,
  OXM_OF_ICMPV6_CODE,
  OXM_OF_IPV6_ND_TARGET,
  OXM_OF_IPV6_ND_SLL,
  OXM_OF_IPV6_ND_TLL,
  OXM_OF_MPLS_LABEL,
  OXM_OF_MPLS_TC,
  OXM_OF_MPLS_BOS,
  OXM_OF_PBB_ISID,
  OXM_OF_TUNNEL_ID,
  0
};

void
test_flowdb_table_features_write_setfield(void) {
  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_WRITE_SETFIELD, struct oxm_id, oxm_id_list,
                              ofp.id, set_field_oxm_ids);
}

void
test_flowdb_table_features_apply_setfield(void) {
  TEST_SCENARIO_FEATURE_CHECK(OFPTFPT_APPLY_SETFIELD, struct oxm_id, oxm_id_list,
                              ofp.id, set_field_oxm_ids);
}

/* XXX experimenter is not supported yet. */
