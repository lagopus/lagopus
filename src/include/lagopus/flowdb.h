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
 * @file        flowdb.h
 * @brief       Flow database.
 */

#ifndef SRC_INCLUDE_LAGOPUS_FLOWDB_H_
#define SRC_INCLUDE_LAGOPUS_FLOWDB_H_

#include "lagopus_apis.h"
#include "openflow.h"
#include "ofcache.h"

struct channel;
struct bridge;
struct lagopus_packet;
struct flow_stats_list;
struct table_stats_list;
struct table_features_list;
struct group_table;

/**
 * @brief Match structure.
 */
struct match {
  TAILQ_ENTRY(match) entry;     /** link for next match */
  bool except_flag;             /** used flag, except for baseic match */
  uint16_t oxm_class;           /** OXM Class */
  uint8_t oxm_field;            /** OXM field */
  uint8_t oxm_length;           /** OXM length */
  uint8_t oxm_value[0];         /** OXM value */
};

TAILQ_HEAD(match_list, match);  /** Match list. */

/**
 * @brief Encap/Decap property structure.
 */
struct ed_prop {
  TAILQ_ENTRY(ed_prop) entry;
  union {
    struct ofp_ed_prop_header ofp_ed_prop;
    struct ofp_ed_prop_portname ofp_ed_prop_portname;
  };
};

TAILQ_HEAD(ed_prop_list, ed_prop); /* list of ed_prop. */

/**
 * @brief Action structure.
 */
struct action {
  TAILQ_ENTRY(action) entry;    /** link for next action */
  lagopus_result_t (*exec)(struct lagopus_packet *,
                           struct action *);
  uint64_t cookie;              /** cookie for packet_in */
  int flags;
  struct ed_prop_list ed_prop_list;
  struct ofp_action_header ofpat;
};

TAILQ_HEAD(action_list, action);        /** list of action. */


/**
 * execution order. see 5.10 Action Set
 */
#define LAGOPUS_ACTION_SET_ORDER_OUTPUT 10
#define LAGOPUS_ACTION_SET_ORDER_MAX 11

/**
 * @brief Instruction structure.
 */
struct instruction {
  TAILQ_ENTRY(instruction) entry;       /** link for next instruction */
  union {
    struct ofp_instruction ofpit;       /** common instruction structure */
    struct ofp_instruction_goto_table ofpit_goto_table;
    struct ofp_instruction_write_metadata ofpit_write_metadata;
    struct ofp_instruction_actions ofpit_actions;
    struct ofp_instruction_meter ofpit_meter;
    struct ofp_instruction_experimenter ofpit_experimenter;
  };
  struct action_list action_list;

  lagopus_result_t (*exec)(struct lagopus_packet *,
                           const struct instruction *);
};

enum {
  INSTRUCTION_INDEX_METER = 0,
  INSTRUCTION_INDEX_APPLY_ACTIONS,
  INSTRUCTION_INDEX_CLEAR_ACTIONS,
  INSTRUCTION_INDEX_WRITE_ACTIONS,
  INSTRUCTION_INDEX_WRITE_METADATA,
  INSTRUCTION_INDEX_GOTO_TABLE,
  INSTRUCTION_INDEX_MAX
};

enum {
  OOB_BASE,
  ETH_BASE,
  PBB_BASE,
  MPLS_BASE,
  L3_BASE,
  IPPROTO_BASE,
  L4_BASE,
  L4P_BASE,
  OOB2_BASE,
  V6SRC_BASE,
  V6DST_BASE,
  NDSLL_BASE,
  NDTLL_BASE,
  MAX_BASE
};

/**
 * @brief Byte offset match structure.
 */
struct byteoff_match {
  uint32_t bits;
  uint8_t bytes[32];
  uint8_t masks[32];
};

/**
 * @brief Out Of Bound data structure.  size are equal or less then 32byte.
 */
struct oob_data {
  uint64_t metadata;            /** OpenFlow metadata */
  uint32_t in_port;             /** OpenFlow ingress port */
  uint32_t in_phy_port;         /** OpenFlow ingress physical port */
  uint32_t packet_type;         /** OpenFlow packet type */
  uint16_t ether_type;          /** L3 ethertype */
  uint16_t vlan_tci;            /** VLAN TCI */
};

/**
 * @brief Out Of Bound data (2nd).  size are equal or less then 32byte.
 */
struct oob2_data {
  uint64_t tunnel_id;           /** OpenFlow Tunnel ID */
  uint16_t ipv6_exthdr;         /** OpenFlow IPV6_EXT pseudo header */
};

TAILQ_HEAD(instruction_list, instruction);      /** Instruction list. */

/**
 * @brief Flow entry.
 */
struct flow {
  /* Match structure used in dataplane. */
  struct byteoff_match byteoff_match[MAX_BASE];

  /* Instruction array for execution. */
  struct instruction *instruction[INSTRUCTION_INDEX_MAX];
  uint64_t packet_count;                        /** Matched packet count. */
  uint64_t byte_count;                          /** Mtched byte count. */
  uint16_t flags;                               /** ofp_flow_mod flags. */
  int32_t priority;                             /** Priority. */
  uint64_t cookie;                              /** ofp_flow_mod cookie. a*/
  uint16_t idle_timeout;                        /** Idle timeout. */
  uint16_t hard_timeout;                        /** Hard timeout. */
  struct match_list match_list;                 /** Match list. */
  struct instruction_list instruction_list;     /** Instruction list. */
  struct bridge *bridge;                        /** Pointer to bridge. */
  uint64_t field_bits;                          /** Match field type bits. */
  uint8_t table_id;                             /** Table ID. */
  struct timespec create_time;                  /** Creation time. */
  struct timespec update_time;                  /** Last updated time. */
  struct flow **flow_timer;                     /** Back reference to entry
                                                 ** of the flow timer. */

};

enum action_flag {
  SET_FIELD_ETH_DST    = 1 << 0,
  SET_FIELD_ETH_SRC    = 1 << 1,
  OUTPUT_PACKET        = 1 << 2,
  OUTPUT_COPIED_PACKET = 1 << 3
};
#define SET_FIELD_ETH (SET_FIELD_ETH_DST | SET_FIELD_ETH_SRC)

struct flowinfo;
struct thtable;

/**
 * @brief List of flow entries.
 */
struct flow_list {
  int nflow;
  struct flow **flows;
  int alloced;

  /* decision tree */
  uint8_t type;
  uint8_t base;
  uint8_t match_off;
  uint8_t keylen;
  union {
    uint8_t match_mask8;
    uint16_t match_mask16;
    uint32_t match_mask32;
    uint64_t match_mask64;
    uint8_t mask[sizeof(uint64_t)];
  };
  union {
    uint8_t min8;
    uint16_t min16;
    uint32_t min32;
    uint64_t min64;
  };
  union {
    uint8_t threshold8;
    uint16_t threshold16;
    uint32_t threshold32;
    uint64_t threshold64;
  };
  struct flow_list *flows_dontcare;
  struct flowinfo *basic;

  uint8_t oxm_field;
  uint8_t shift;
  struct flow_list **update_timer;
  struct thtable *thtable;

  int nbranch;
  void *branch[0];
};

/**
 * @brief Flow table.
 */
struct table {
  struct flow_list *flow_list;  /** Flows by types. */
  uint64_t lookup_count;        /** Lookup count */
  uint64_t matched_count;       /** Matched count */
  uint8_t table_id;             /** Table id. */
  uint16_t flow_match_type_count[OFPXMT_OFB_IPV6_EXTHDR + 1];  /** Flow counts
                                                                ** by match
                                                                ** type. */
  struct ofp_table_features features;   /** Features. */
  void *userdata;               /** userdata used in dataplane */
};


#define FLOWDB_TABLE_SIZE_MAX   255

enum switch_mode {
  SWITCH_MODE_OPENFLOW = 0,
  SWITCH_MODE_SECURE = 1,
  SWITCH_MODE_STANDALONE = 2
};

struct flowdb;

void (*lagopus_register_action_hook)(struct action *);
void (*lagopus_register_instruction_hook)(struct instruction *);
void (*lagopus_add_flow_hook)(struct flow *, struct table *);
void (*lagopus_del_flow_hook)(struct flow *, struct table *);
struct flow *(*lagopus_find_flow_hook)(struct flow *, struct table *);

/**
 * Allocate a new flow database.
 *
 * @param[in]   table_size      Table size of the flow database.
 *
 * @retval NULL Allocation failed.
 * @retval Non-NULL Allocation success.
 */
struct flowdb *
flowdb_alloc(uint8_t table_size);

/**
 * Free flow database.
 *
 * @param[in]   flowdb  Flow database to be freed.
 */
void
flowdb_free(struct flowdb *flowdb);

/**
 * Get current switch mode (openflow, secure or standalone).
 *
 * @param[in]   flowdb  Flow database.
 * @param[out]  switch_mode     Current switch_mode.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
flowdb_switch_mode_get(struct flowdb *flowdb, enum switch_mode *switch_mode);

/**
 * Set switch mode (openflow, secure or standalone).
 *
 * @param[in]   flowdb  Flow database.
 * @param[in]  switch_mode      switch_mode to be set.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
flowdb_switch_mode_set(struct flowdb *flowdb, enum switch_mode switch_mode);

/**
 * Add flow entry to the flow database.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   flow_mod        ofp_flow_mod structure of the flow.
 * @param[in]   match   ofp_match structure of the flow.
 * @param[in]   instruction ofp_instruction of the flow.
 * @param[out]  error   OFP_ERROR value.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Failed, invalid argument(s).
 * @retval LAGOPUS_RESULT_NO_MEMORY     Failed, no memory.
 * @retval LAGOPUS_RESULT_OFP_ERROR     Failed with OFP error message.
 */
lagopus_result_t
flowdb_flow_add(struct bridge *bridge,
                struct ofp_flow_mod *flow_mod,
                struct match_list *match_list,
                struct instruction_list *instruction_list,
                struct ofp_error *error);

/**
 * Modify flow entry of the flow database.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   flow_mod        ofp_flow_mod structure of the flow.
 * @param[in]   match_list      list of match structures.
 * @param[in]   instruction_list        list of instruction structures.
 * @param[out]  error   OFP_ERROR value.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Failed, invalid argument(s).
 * @retval LAGOPUS_RESULT_NO_MEMORY     Failed, no memory.
 * @retval LAGOPUS_RESULT_OFP_ERROR     Failed with OFP error message.
 */
lagopus_result_t
flowdb_flow_modify(struct bridge *bridge,
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   struct ofp_error *error);

/**
 * Delete flow entry from the flow database.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   flow_mod        ofp_flow_mod structure of the flow.
 * @param[in]   match_list      list of match structures.
 * @param[out]  error   OFP_ERROR value.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Failed, invalid argument(s).
 * @retval LAGOPUS_RESULT_NO_MEMORY     Failed, no memory.
 * @retval LAGOPUS_RESULT_OFP_ERROR     Failed with OFP error message.
 */
lagopus_result_t
flowdb_flow_delete(struct bridge *bridge,
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct ofp_error *error);

/**
 * Get stats of flows from the flow database.
 *
 * @param[in]   flowdb  Flow database.
 * @param[in]   request ofp_flow_stats_request structure of the flow.
 * @param[in]   match_list list of match structures.
 * @param[out]  flow_stats_list  matched flow statistics.
 * @param[out]  error   OFP_ERROR value.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Failed, invalid argument(s).
 * @retval LAGOPUS_RESULT_NO_MEMORY     Failed, no memory.
 * @retval LAGOPUS_RESULT_OFP_ERROR     Failed with OFP error message.
 */
lagopus_result_t
flowdb_flow_stats(struct flowdb *flowdb,
                  struct ofp_flow_stats_request *request,
                  struct match_list *match_list,
                  struct flow_stats_list *flow_stats_list,
                  struct ofp_error *error);

/**
 * Get aggregated stats of flows from the flow database.
 *
 * @param[in]   flowdb  Flow database.
 * @param[in]   request ofp_flow_stats_request structure of the flow.
 * @param[in]   match_list list of match structures.
 * @param[out]  flow_stats_list  matched flow statistics.
 * @param[out]  error   OFP_ERROR value.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Failed, invalid argument(s).
 * @retval LAGOPUS_RESULT_NO_MEMORY     Failed, no memory.
 * @retval LAGOPUS_RESULT_OFP_ERROR     Failed with OFP error message.
 */
lagopus_result_t
flowdb_aggregate_stats(struct flowdb *,
                       struct ofp_aggregate_stats_request *,
                       struct match_list *,
                       struct ofp_aggregate_stats_reply *,
                       struct ofp_error *);

lagopus_result_t
flowdb_table_stats(struct flowdb *,
                   struct table_stats_list *,
                   struct ofp_error *);

lagopus_result_t
flowdb_get_table_features(struct flowdb *,
                          struct table_features_list *,
                          struct ofp_error *);

/**
 * Add flow to flow list.
 *
 * @param[in]   flow    Flow entry.
 * @param[in]   flows   Flow list.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
flow_add_sub(struct flow *flow, struct flow_list *flows);

/**
 * Dump flow as human readable.
 *
 * @param[in]   flow    Flow entry.
 * @param[in]   fp      Output file pointer.
 */
void
flow_dump(struct flow *flow, FILE *fp);

/**
 * Dump all flows as human readable in flowdb.
 *
 * @param[in]   flowdb  Flow database.
 * @param[in]   fp      Output file pointer.
 */
void
flowdb_dump(struct flowdb *flowdb, FILE *fp);

/**
 * Get table from flowdb.
 *
 * @param[in]   flowdb          Flow database.
 * @param[in]   table_id        Table ID.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_NOT_FOUND     Table not found.
 */
struct table *
flowdb_get_table(struct flowdb *flowdb, uint8_t table_id);

/* Utility functions. */
void
match_list_entry_free(struct match_list *match_list);

void
action_list_entry_free(struct action_list *action_list);

void
instruction_list_entry_free(struct instruction_list *instruction_list);

uint32_t
match_mpls_label_host_get(struct match *match);

lagopus_result_t
copy_action_list(struct action_list *dst, const struct action_list *src);

void flow_remove_group_action(struct flow *flow);

lagopus_result_t
flow_remove_with_reason(struct flow *flow,
                        struct bridge *bridge,
                        uint8_t reason,
                        struct ofp_error *error);

/**
 * no lock version of flow_remove_with_reason.
 */
lagopus_result_t
flow_remove_with_reason_nolock(struct flow *flow,
                               struct bridge *bridge,
                               uint8_t reason,
                               struct ofp_error *error);

struct timespec now_ts;

static inline struct timespec
get_current_time(void) {
  struct timespec ts;

  /* XXX lock */
  clock_gettime(CLOCK_MONOTONIC, &now_ts);
  ts = now_ts;

  return ts;
}

/**
 * initialize flow timer related structure.
 */
void init_dp_timer(void);

/**
 * register flow with timeout to timer list.
 *
 * @param[in]   flow    flow.
 */
lagopus_result_t add_flow_timer(struct flow *flow);

/**
 * register to refresh flow_list optimization to timer list.
 *
 * @param[in]   flow_list       list of flow.
 */
lagopus_result_t add_mbtree_timer(struct flow_list *flow_list, time_t timeout);

/**
 * flow removal timer loop.
 */
lagopus_result_t dp_timer_loop(const lagopus_thread_t *t, void *arg);

/**
 * Free match list elements.
 *
 * @param[in]   match_list      List of \e math structures.
 *
 * @retval      void
 */
void
ofp_match_list_elem_free(struct match_list *match_list);

/**
 * Free instruction     list elements.
 *
 * @param[in]   instruction_list        List of \e instruction structures.
 *
 * @retval      void
 */
void
ofp_instruction_list_elem_free(struct instruction_list *instruction_list);

/**
 * Free action list elements.
 *
 * @param[in]   action_list     List of \e action structures.
 *
 * @retval      void
 */
void
ofp_action_list_elem_free(struct action_list *action_list);

/**
 * Lookup flow table.
 *
 * @param[in]   flowdb          Flow database.
 * @param[in]   table_id        OpenFlow table id.
 *
 * @retval      !=NULL          Flow table.
 * @retval      ==NULL          table is not found.
 */
struct table *table_lookup(struct flowdb *flowdb, uint8_t table_id);

/**
 * Copy match list.
 *
 * @param[out]  dst             Copy destination of match list.
 * @param[in]   src             souce match list.
 *
 * @retval      LAGOPUS_RESULT_OK         Success.
 * @retval      LAGOPUS_RESULT_NO_MEMORY  Memory exhausted.
 *
 * dislike TAILQ_CONCAT, src is not modified and copied contents.
 */
lagopus_result_t
copy_match_list(struct match_list *dst,
                const struct match_list *src);

#endif /* SRC_INCLUDE_LAGOPUS_FLOWDB_H_ */
