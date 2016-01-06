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
 * @file	ofp_multipart_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_multipart
 * @details	Describe APIs between Agent and Data-Plane for ofp_multipart.
 */
#ifndef __LAGOPUS_OFP_MULTIPART_APIS_H__
#define __LAGOPUS_OFP_MULTIPART_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/bridge.h"
#include "lagopus/pbuf.h"


/* Multipart - Flow Stats */
/**
 * Flow stats.
 */
struct flow_stats {
  TAILQ_ENTRY(flow_stats) entry;
  struct ofp_flow_stats ofp;
  struct match_list match_list;
  struct instruction_list instruction_list;
};

/**
 * Flow stats list.
 */
TAILQ_HEAD(flow_stats_list, flow_stats);

/**
 * Get array of flow statistics for \b OFPMP_FLOW.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	flow_stats_reques	A pointer to \e ofp_flow_stats_reques
 *     structure.
 *     @param[in]       match_list      A pointer to list of match.
 *     @param[out]	flow_stats_list	A pointer to list of flow stats.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_flow_stats_get(uint64_t dpid,
                   struct ofp_flow_stats_request *flow_stats_request,
                   struct match_list *match_list,
                   struct flow_stats_list *flow_stats_list,
                   struct ofp_error *error);
/* Multipart - Flow Stats END */

/* Multipart - Queue stats */
/**
 * Queue stats.
 */
struct queue_stats {
  TAILQ_ENTRY(queue_stats) entry;
  struct ofp_queue_stats ofp;
};

/**
 * Queue stats list.
 */
TAILQ_HEAD(queue_stats_list, queue_stats);

/**
 * Get array of queue statistics for \b OFPMP_QUEUE.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	queue_stats_request	A pointer to \e ofp_queue_stats_request
 *     structure.
 *     @param[out]	ofp_queue_stats_list	A pointer to list of queue stats.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_queue_stats_request_get(uint64_t dpid,
                            struct ofp_queue_stats_request *queue_stats_request,
                            struct queue_stats_list *queue_stats_list,
                            struct ofp_error *error);
/* Multipart - Queue stats END */

/* Group stats */
/**
 * Bucket counter.
 */
struct bucket_counter {
  TAILQ_ENTRY(bucket_counter) entry;
  struct ofp_bucket_counter ofp;
};

/**
 * Bucket counter list.
 */
TAILQ_HEAD(bucket_counter_list, bucket_counter);

/**
 * Group stats.
 */
struct group_stats {
  TAILQ_ENTRY(group_stats) entry;
  struct ofp_group_stats ofp;
  struct bucket_counter_list bucket_counter_list;
};

/**
 * Group stats list.
 */
TAILQ_HEAD(group_stats_list, group_stats);

/**
 * Get array of group statistics for \b OFPMP_GROUP.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	group_stats_request	A pointer to \e ofp_group_stats_request
 *     structure.
 *     @param[out]	group_stats_list	A pointer to list of group stats.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_group_stats_request_get(uint64_t dpid,
                            struct ofp_group_stats_request *group_stats_request,
                            struct group_stats_list *group_stats_list,
                            struct ofp_error *error);
/* Group stats END */

/* Multipart - Table stats */
/**
 * Table stats.
 */
struct table_stats {
  TAILQ_ENTRY(table_stats) entry;
  struct ofp_table_stats ofp;
};

/**
 * Table stats list.
 */
TAILQ_HEAD(table_stats_list, table_stats);

/**
 * Get array of all table statistics for \b OFPMP_TABLE.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	table_stats_list	A pointer to list of table stats.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_table_stats_get(uint64_t dpid,
                    struct table_stats_list *table_stats_list,
                    struct ofp_error *error);
/* Multipart - Table stats END */

/* Multipart - Aggregate stats */
/**
 * Get a aggregate flow statistics for \b OFPMP_AGGREGATE.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	aggre_request	A pointer to \e ofp_aggregate_stats_request
 *     structure.
 *     @param[in]	match_list	A pointer to list of match.
 *     @param[out]	aggre_reply	A pointer to \e ofp_aggregate_stats_reply
 *     structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_aggregate_stats_reply_get(uint64_t dpid,
                              struct ofp_aggregate_stats_request *aggre_request,
                              struct match_list *match_list,
                              struct ofp_aggregate_stats_reply *aggre_reply,
                              struct ofp_error *error);
/* Multipart - Aggregate stats END */

/* Multipart - Table features */
/**
 * Experimenter data.
 */
struct experimenter_data {
  TAILQ_ENTRY(experimenter_data) entry;
  struct ofp_experimenter_data ofp;
};

/**
 * Experimenter data list.
 */
TAILQ_HEAD(experimenter_data_list, experimenter_data);

/**
 * OXM id.
 */
struct oxm_id {
  TAILQ_ENTRY(oxm_id) entry;
  struct ofp_oxm_id ofp;
};

/**
 * OXM id list.
 */
TAILQ_HEAD(oxm_id_list, oxm_id);

/**
 * Next table id.
 */
struct next_table_id {
  TAILQ_ENTRY(next_table_id) entry;
  struct ofp_next_table_id ofp;
};

/**
 * Next table id list.
 */
TAILQ_HEAD(next_table_id_list, next_table_id);

/**
 * Table property.
 */
struct table_property {
  TAILQ_ENTRY(table_property) entry;
  struct ofp_table_feature_prop_header ofp;

  /* for ofp_table_feature_prop_instructions. */
  struct instruction_list instruction_list;

  /* for ofp_table_feature_prop_next_tables. */
  struct next_table_id_list next_table_id_list;

  /* for ofp_table_feature_prop_actions. */
  struct action_list action_list;

  /* for ofp_table_feature_prop_oxm. */
  struct oxm_id_list oxm_id_list;

  /* for ofp_table_feature_prop_experimenter. */
  uint32_t  experimenter;
  uint32_t exp_type;
  struct experimenter_data_list experimenter_data_list;
};

/**
 * Table property list.
 */
TAILQ_HEAD(table_property_list, table_property);

/**
 * Table features.
 */
struct table_features {
  TAILQ_ENTRY(table_features) entry;
  struct ofp_table_features ofp;
  struct table_property_list table_property_list;
};

/**
 * Table features list.
 */
TAILQ_HEAD(table_features_list, table_features);

/**
 * Get array of all table features for \b OFPMP_TABLE_FEATURES.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	table_features_list	A pointer to list of table features.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_table_features_get(uint64_t dpid,
                       struct table_features_list *table_features_list,
                       struct ofp_error *error);

/**
 * Set table features for \b OFPMP_TABLE_FEATURES.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	table_features_list	A pointer to list of table features.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Data-Plan side.
 */
lagopus_result_t
ofp_table_features_set(uint64_t dpid,
                       struct table_features_list *table_features_list,
                       struct ofp_error *error);
/* Multipart - Table features END */

/* Port Stats */
/**
 * Port stats.
 */
struct port_stats {
  TAILQ_ENTRY(port_stats) entry;
  struct ofp_port_stats ofp;
};

/**
 * Port stats list.
 */
TAILQ_HEAD(port_stats_list, port_stats);

/**
 * Get array of port statistics for \b OFPMP_PORT.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	port_stats_request	A pointer to \e ofp_port_stats_request.
 *     structure.
 *     @param[out]	port_stats_list	A pointer to list of port stats.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_port_stats_request_get(uint64_t dpid,
                           struct ofp_port_stats_request *port_stats_request,
                           struct port_stats_list *port_stats_list,
                           struct ofp_error *error);
/* Port Stats END */

/* Group desc */
struct group_desc {
  TAILQ_ENTRY(group_desc) entry;
  struct ofp_group_desc ofp;
  struct bucket_list bucket_list;
};

/**
 * Group stats list.
 */
TAILQ_HEAD(group_desc_list, group_desc);

/**
 * Get array of group description for \b OFPMP_GROUP.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	group_desc_list	A pointer to list of group desc.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_group_desc_get(uint64_t dpid,
                   struct group_desc_list *group_desc_list,
                   struct ofp_error *error);
/* Group desc END */

/* Group features */
/**
 * Get group features for \b OFPMP_GROUP.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	group_features	A pointer to group features structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_group_features_get(uint64_t dpid,
                       struct ofp_group_features *group_features,
                       struct ofp_error *error);

/* Group features END */

/* Meter stats */
/**
 * Band stats.
 */
struct meter_band_stats {
  TAILQ_ENTRY(meter_band_stats) entry;
  struct ofp_meter_band_stats ofp;
};

/**
 * Band stats list.
 */
TAILQ_HEAD(meter_band_stats_list, meter_band_stats);

/**
 * Meter stats.
 */
struct meter_stats {
  TAILQ_ENTRY(meter_stats) entry;
  struct ofp_meter_stats ofp;
  struct meter_band_stats_list meter_band_stats_list;
};

/**
 * Meter stats list.
 */
TAILQ_HEAD(meter_stats_list, meter_stats);

/**
 * Get array of meter statistics for \b OFPMT_METER.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	meter_multipart_request	A pointer to
 *     \e ofp_meter_multipart_request structures.
 *     @param[out]	meter_stats_list	A pointer to list of meter stats.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_meter_stats_get(uint64_t dpid,
                    struct ofp_meter_multipart_request *meter_request,
                    struct meter_stats_list *meter_stats_list,
                    struct ofp_error *error);
/* Meter stats END */

/* Meter config  */
/**
 * Meter config
 */
struct meter_config {
  TAILQ_ENTRY(meter_config) entry;
  struct ofp_meter_config ofp;
  struct meter_band_list band_list;
};

/**
 * Meter stats list.
 */
TAILQ_HEAD(meter_config_list, meter_config);

/**
 * Get array of meter configuration statistics for \b OFPMT_METER_CONFIG.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	meter_request	A pointer to
 *     \e ofp_meter_multipart_request structures.
 *     @param[out]	meter_config_list	A pointer to list of meter config.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_meter_config_get(uint64_t dpid,
                     struct ofp_meter_multipart_request *meter_request,
                     struct meter_config_list *meter_config_list,
                     struct ofp_error *error);
/* Meter config END */

/* Meter features */
/**
 * Get meter features statistics for \b OFPMT_METER_FEATURES.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	meter_features	A pointer to \e ofp_meter_features
 *     structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_meter_features_get(uint64_t dpid,
                       struct ofp_meter_features *meter_features,
                       struct ofp_error *error);
/* Meter features END */

/* Description */
/**
 * Get description statistics for \b OFPMP_DESC.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	meter_features	A pointer to \e ofp_desc
 *     structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_desc_get(uint64_t dpid,
             struct ofp_desc *desc,
             struct ofp_error *error);
/* Description END */

/* Port description */
struct port_desc {
  TAILQ_ENTRY(port_desc) entry;
  struct ofp_port ofp;
};

/**
 * Port description list.
 */
TAILQ_HEAD(port_desc_list, port_desc);

/**
 * Get port description statistics for \b OFPMP_PORT_DESCRIPTION.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	port_desc_list	A pointer to list of port desc.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_port_get(uint64_t dpid,
             struct port_desc_list *port_desc_list,
             struct ofp_error *error);
/* Port description END */

#endif /* __LAGOPUS_OFP_MULTIPART_APIS_H__ */
