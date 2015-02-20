/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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

#ifdef HAVE_DPDK
#include "rte_rwlock.h"
#endif /* HAVE_DPDK */

#include "lagopus_apis.h"
#include "openflow.h"
#include "ofcache.h"

/*
 * flowdb lock primitive.
 */
#ifdef HAVE_DPDK
rte_rwlock_t flowdb_update_lock;
rte_rwlock_t dpmgr_lock;

#define FLOWDB_RWLOCK_INIT()
#define FLOWDB_RWLOCK_RDLOCK()  do {                                    \
    rte_rwlock_read_lock(&dpmgr_lock);                                  \
  } while(0)
#define FLOWDB_RWLOCK_WRLOCK()  do {                                    \
    rte_rwlock_write_lock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_RDUNLOCK() do {                                   \
    rte_rwlock_read_unlock(&dpmgr_lock);                                \
  } while(0)
#define FLOWDB_RWLOCK_WRUNLOCK() do {                                   \
    rte_rwlock_write_unlock(&dpmgr_lock);                               \
  } while(0)
#define FLOWDB_UPDATE_CHECK() do {                      \
    rte_rwlock_read_lock(&flowdb_update_lock);          \
    rte_rwlock_read_unlock(&flowdb_update_lock);        \
  } while (0)
#define FLOWDB_UPDATE_BEGIN() do {               \
    rte_rwlock_write_lock(&flowdb_update_lock);  \
  } while (0)
#define FLOWDB_UPDATE_END() do {                        \
    rte_rwlock_write_unlock(&flowdb_update_lock);       \
  } while (0)
#else
pthread_rwlock_t flowdb_update_lock;
pthread_rwlock_t dpmgr_lock;
#define FLOWDB_RWLOCK_INIT() do {                                       \
    pthread_rwlock_init(&flowdb_update_lock, NULL);                     \
    pthread_rwlock_init(&dpmgr_lock, NULL);                             \
  } while(0)
#define FLOWDB_RWLOCK_RDLOCK()  do {                                    \
    pthread_rwlock_rdlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_WRLOCK()  do {                                    \
    pthread_rwlock_wrlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_RDUNLOCK() do {                                   \
    pthread_rwlock_unlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_WRUNLOCK() do {                                   \
    pthread_rwlock_unlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_UPDATE_CHECK() do {                      \
    pthread_rwlock_rdlock(&flowdb_update_lock);    \
    pthread_rwlock_unlock(&flowdb_update_lock);     \
  } while (0)
#define FLOWDB_UPDATE_BEGIN() do {                      \
    pthread_rwlock_wrlock(&flowdb_update_lock);    \
  } while (0)
#define FLOWDB_UPDATE_END() do {                        \
    pthread_rwlock_unlock(&flowdb_update_lock);     \
  } while (0)
#endif /* HAVE_DPDK */

struct channel;
struct bridge;
struct lagopus_packet;
struct flow_stats_list;
struct table_stats_list;
struct table_features_list;
struct group_table;
struct vector;

/* Match. */
struct match {
  TAILQ_ENTRY(match) entry;
  bool except_flag;
  uint16_t oxm_class;
  uint8_t oxm_field;
  uint8_t oxm_length;
  uint8_t oxm_value[0];
};

/* Match list. */
TAILQ_HEAD(match_list, match);

/* Action. */
struct action {
  TAILQ_ENTRY(action) entry;
  lagopus_result_t (*exec)(struct lagopus_packet *,
                           struct action *);
  uint64_t cookie; /* for packet_in */
  int flags;
  struct ofp_action_header ofpat;
};

/* list of action. */
TAILQ_HEAD(action_list, action);

/**
 * execution order. see 5.10 Action Set
 */
#define LAGOPUS_ACTION_SET_ORDER_OUTPUT 10
#define LAGOPUS_ACTION_SET_ORDER_MAX 11

/**
 * Instruction.
 */
struct instruction {
  TAILQ_ENTRY(instruction) entry;
  union {
    struct ofp_instruction ofpit;
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
  ETH_BASE,
  PBB_BASE,
  MPLS_BASE,
  L3_BASE,
  IPPROTO_BASE,
  L4_BASE,
  NDSLL_BASE,
  NDTLL_BASE,
  MAX_BASE
};

struct byteoff_match {
  uint64_t bits;
  uint8_t bytes[64];
  uint8_t masks[64];
};

struct oob_data {
  uint32_t in_port;
  uint32_t in_phy_port;
  uint16_t vlan_tci;
  uint64_t metadata;
  uint64_t tunnel_id;
  uint16_t ipv6_exthdr;
};

/* Action list. */
TAILQ_HEAD(instruction_list, instruction);

/* Flow entry. */
struct flow {
  /* Match structure used in datapath. */
  struct byteoff_match oob_match;
  struct byteoff_match byteoff_match[MAX_BASE];

  /* Instruction array for execution. */
  struct instruction *instruction[INSTRUCTION_INDEX_MAX];

  /* Statistics. */
  uint64_t packet_count;
  uint64_t byte_count;

  /* ofp_flow_mod flags. */
  uint16_t flags;

  /* Priority. */
  int32_t priority;

  /* ofp_flow_mod cookie. a*/
  uint64_t cookie;

  /* Timeouts. */
  uint16_t idle_timeout;
  uint16_t hard_timeout;

  /* Match list. */
  struct match_list match_list;

  /* Instruction list for statistics. */
  struct instruction_list instruction_list;

  /* Pointer to bridge. */
  struct bridge *bridge;

  /* Match field type bits. */
  uint64_t field_bits;

  /* Table ID. */
  uint8_t table_id;

  /* Internal use of flow type. */
  uint8_t flow_type;

  /* Creation time. */
  struct timespec create_time;

  /* Last updated time. */
  struct timespec update_time;

  /* Back reference to entry of the flow timer. */
  struct flow **flow_timer;
};

enum flow_index {
  ARP_FLOWS = 0,
  VLAN_FLOWS,
  IPV4_FLOWS,
  IPV6_FLOWS,
  MPLS_FLOWS,
  MPLSMC_FLOWS,
  PBB_FLOWS,
  MISC_FLOWS,
  MAX_FLOWS
};

enum action_flag {
  SET_FIELD_ETH_DST    = 1 << 0,
  SET_FIELD_ETH_SRC    = 1 << 1,
  OUTPUT_PACKET        = 1 << 2,
  OUTPUT_COPIED_PACKET = 1 << 3
};
#define SET_FIELD_ETH (SET_FIELD_ETH_DST | SET_FIELD_ETH_SRC)

struct flow_list {
  int nflow;
  struct flow **flows;

  /* MPLS match patricia tree. */
  struct ptree *mpls_tree;
};

/* Flow table. */
struct table {
  /* Flows by types. */
  struct flow_list flows[MAX_FLOWS];

  /* active_count is number of flows. */
  uint64_t lookup_count;
  uint64_t matched_count;

  uint8_t table_id;

  /* Flow counts by match type. */
  uint16_t flow_match_type_count[OFPXMT_OFB_IPV6_EXTHDR + 1];

  /* Features. */
  struct ofp_table_features features;

  /* userdata used in datapath */
  void *userdata;
};


#define FLOWDB_TABLE_SIZE_MAX   255

enum switch_mode {
  SWITCH_MODE_OPENFLOW = 0,
  SWITCH_MODE_SECURE = 1,
  SWITCH_MODE_STANDALONE = 2
};

/* Flow database. */
struct flowdb {
  /* Read-write lock. */
#ifdef HAVE_DPDK
  rte_rwlock_t rwlock;
#else
  pthread_rwlock_t rwlock;
#endif /* HAVE_DPDK */

  /* Flow table size. */
  uint8_t table_size;

  /* Flow table. */
  struct table **tables;

  /* Switch mode. */
  enum switch_mode switch_mode;
};

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
 * Initialize lock of the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
static inline void
flowdb_lock_init(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_INIT();
}

/**
 * Read lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
static inline void
flowdb_rdlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_RDLOCK();
}

/**
 * Check write lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
static inline void
flowdb_check_update(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_UPDATE_CHECK();
}

/**
 * Write lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
static inline void
flowdb_wrlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_UPDATE_BEGIN();
  FLOWDB_RWLOCK_WRLOCK();
}

/**
 * Unlock read lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be unlocked.
 */
static inline void
flowdb_rdunlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_RDUNLOCK();
}

/**
 * Unlock write lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be unlocked.
 */
static inline void
flowdb_wrunlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_WRUNLOCK();
  FLOWDB_UPDATE_END();
}

/* Temporary flowdb dump functions. */
void
flow_dump(struct flow *flow, FILE *fp);

void
flowdb_dump(struct flowdb *flowdb, FILE *fp);

/* Get table from flowdb. */
struct table *
table_get(struct flowdb *flowdb, uint8_t table_id);

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
void init_flow_timer(void);

/**
 * register flow with timeout to timer list.
 *
 * @param[in]   flow    flow.
 */
lagopus_result_t add_flow_timer(struct flow *flow);

/**
 * flow removal timer loop.
 */
lagopus_result_t flow_timer_loop(const lagopus_thread_t *t, void *arg);

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
static inline struct table *
table_lookup(struct flowdb *flowdb, uint8_t table_id) {
  return flowdb->tables[table_id];
}

#endif /* SRC_INCLUDE_LAGOPUS_FLOWDB_H_ */
