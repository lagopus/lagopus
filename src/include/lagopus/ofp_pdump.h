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
 * @file	ofp_pdump.h
 * @brief	Packet dumper.
 */

#ifndef __LAGOPUS_OFP_PDUMP_H__
#define __LAGOPUS_OFP_PDUMP_H__

/**
 * @brief	trace_pdump_type
 * @details	Type of trace pdump.
 */
enum trace_pdump_type {
  /* OFP header. */
  PDUMP_OFP_HEADER,
  /* OFP type. */
  PDUMP_OFP_HELLO,
  PDUMP_OFP_ERROR,
  PDUMP_OFP_ECHO_REQUEST,
  PDUMP_OFP_ECHO_REPLY,
  PDUMP_OFP_EXPERIMENTER,
  PDUMP_OFP_FEATURES_REQUEST,
  PDUMP_OFP_FEATURES_REPLY,
  PDUMP_OFP_GET_CONFIG_REQUEST,
  PDUMP_OFP_GET_CONFIG_REPLY,
  PDUMP_OFP_SET_CONFIG,
  PDUMP_OFP_PACKET_IN,
  PDUMP_OFP_FLOW_REMOVED,
  PDUMP_OFP_PORT_STATUS,
  PDUMP_OFP_PACKET_OUT,
  PDUMP_OFP_FLOW_MOD,
  PDUMP_OFP_GROUP_MOD,
  PDUMP_OFP_PORT_MOD,
  PDUMP_OFP_TABLE_MOD,
  PDUMP_OFP_MULTIPART_REQUEST,
  PDUMP_OFP_MULTIPART_REPLY,
  PDUMP_OFP_BARRIER_REQUEST,
  PDUMP_OFP_BARRIER_REPLY,
  PDUMP_OFP_QUEUE_GET_CONFIG_REQUEST,
  PDUMP_OFP_QUEUE_GET_CONFIG_REPLY,
  PDUMP_OFP_ROLE_REQUEST,
  PDUMP_OFP_ROLE_REPLY,
  PDUMP_OFP_GET_ASYNC_REQUEST,
  PDUMP_OFP_GET_ASYNC_REPLY,
  PDUMP_OFP_SET_ASYNC,
  PDUMP_OFP_METER_MOD,
  /* instruction. */
  PDUMP_OFP_INSTRUCTION_GOTO_TABLE,
  PDUMP_OFP_INSTRUCTION_WRITE_METADATA,
  PDUMP_OFP_INSTRUCTION_ACTIONS,
  PDUMP_OFP_INSTRUCTION_METER,
  PDUMP_OFP_INSTRUCTION_EXPERIMENTER,
  /* action. */
  PDUMP_OFP_ACTION_OUTPUT,
  PDUMP_OFP_ACTION_HEADER,
  PDUMP_OFP_ACTION_MPLS_TTL,
  PDUMP_OFP_ACTION_PUSH,
  PDUMP_OFP_ACTION_POP_MPLS,
  PDUMP_OFP_ACTION_SET_QUEUE,
  PDUMP_OFP_ACTION_GROUP,
  PDUMP_OFP_ACTION_NW_TTL,
  PDUMP_OFP_ACTION_SET_FIELD,
  PDUMP_OFP_ACTION_EXPERIMENTER_HEADER,
  /* meter_band. */
  PDUMP_OFP_METER_BAND_DROP,
  PDUMP_OFP_METER_BAND_DSCP_REMARK,
  PDUMP_OFP_METER_BAND_EXPERIMENTER,
  /* bucket. */
  PDUMP_OFP_BUCKET,
  /* match. */
  PDUMP_OFP_MATCH,
  /* packet_queue */
  PDUMP_OFP_PACKET_QUEUE,
  /* queue_prop */
  PDUMP_OFP_QUEUE_PROP_HEADER,
  PDUMP_OFP_QUEUE_PROP_MIN_RATE,
  PDUMP_OFP_QUEUE_PROP_MAX_RATE,
  PDUMP_OFP_QUEUE_PROP_EXPERIMENTER,
  /* flow_stats */
  PDUMP_OFP_FLOW_STATS,
  /* queue_stats */
  PDUMP_OFP_QUEUE_STATS,
  /* bucket_counter */
  PDUMP_OFP_BUCKET_COUNTER,
  /* group_stats */
  PDUMP_OFP_GROUP_STATS,
  /* table_stats */
  PDUMP_OFP_TABLE_STATS,
  /* experimenter_data */
  PDUMP_OFP_EXPERIMENTER_DATA,
  /* oxm_id */
  PDUMP_OFP_OXM_ID,
  /* next_table_id */
  PDUMP_OFP_NEXT_TABLE_ID,
  /* table_features */
  PDUMP_OFP_TABLE_FEATURES,
  /* table_feature_prop */
  PDUMP_OFP_TABLE_FEATURE_PROP_HEADER,
  PDUMP_OFP_TABLE_FEATURE_PROP_INSTRUCTIONS,
  PDUMP_OFP_TABLE_FEATURE_PROP_NEXT_TABLES,
  PDUMP_OFP_TABLE_FEATURE_PROP_ACTIONS,
  PDUMP_OFP_TABLE_FEATURE_PROP_OXM,
  PDUMP_OFP_TABLE_FEATURE_PROP_EXPERIMENTER,
  /* port_stats */
  PDUMP_OFP_PORT_STATS,
  /* group_desc */
  PDUMP_OFP_GROUP_DESC,
  /* meter_stats */
  PDUMP_OFP_METER_STATS,
  /* meter_band_stats */
  PDUMP_OFP_METER_BAND_STATS,
  /* meter_config */
  PDUMP_OFP_METER_CONFIG,
  /* port */
  PDUMP_OFP_PORT,

  /* max. */
  PDUMP_MAX,
};

/**
 * Call trace func.
 *
 *     @param[in]	type	Type of pdump (PDUMP_OFP_*).
 *     @param[in]	pac	A pointer to \e ofp_* structure.
 *     @param[in]	pac_size	Size of \e ofp_* structure.
 *     @param[out]	str	A pointer to \e trace string.
 *     @param[in]	max_len	Max length.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Over max length.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
trace_call(enum trace_pdump_type type,
           const void *pac, size_t pac_size,
           char *str, size_t max_len);

#endif /* __LAGOPUS_OFP_PDUMP_H__ */
