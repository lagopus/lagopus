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

#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "lagopus/ofp_pdump.h"
#include "openflow13packet.h"
#include "ofp_match.h"

struct {
  lagopus_result_t (*func) (const void *pac, size_t pac_size,
                            char *str, size_t max_len);
} trace_pdump[PDUMP_MAX] = {
  /* OFP header. */
  {ofp_header_trace,}, /* PDUMP_OFP_HEADER */
  /* OFP type. */
  {ofp_hello_trace,}, /* PDUMP_OFP_HELLO */
  {ofp_error_msg_trace,}, /* PDUMP_OFP_ERROR */
  {ofp_header_trace,}, /* PDUMP_OFP_ECHO_REQUEST */
  {ofp_header_trace,}, /* PDUMP_OFP_ECHO_REPLY */
  {ofp_experimenter_header_trace,}, /* PDUMP_OFP_EXPERIMENTER */
  {ofp_header_trace,}, /* PDUMP_OFP_FEATURES_REQUEST */
  {ofp_switch_features_trace,}, /* PDUMP_OFP_FEATURES_REPLY */
  {ofp_header_trace,}, /* PDUMP_OFP_GET_CONFIG_REQUEST */
  {ofp_switch_config_trace,}, /* PDUMP_OFP_GET_CONFIG_REPLY */
  {ofp_switch_config_trace,},/* PDUMP_OFP_SET_CONFIG */
  {ofp_packet_in_trace,}, /* PDUMP_OFP_PACKET_IN */
  {ofp_flow_removed_trace,}, /* PDUMP_OFP_FLOW_REMOVED */
  {ofp_port_status_trace,}, /* PDUMP_OFP_PORT_STATUS */
  {ofp_packet_out_trace,}, /* PDUMP_OFP_PACKET_OUT */
  {ofp_flow_mod_trace,}, /* PDUMP_OFP_FLOW_MOD */
  {ofp_group_mod_trace,},/* PDUMP_OFP_GROUP_MOD */
  {ofp_port_mod_trace,}, /* PDUMP_OFP_PORT_MOD */
  {ofp_table_mod_trace,}, /* PDUMP_OFP_TABLE_MOD */
  {ofp_multipart_request_trace,}, /* PDUMP_OFP_MULTIPART_REQUEST */
  {ofp_multipart_reply_trace,},  /* PDUMP_OFP_MULTIPART_REPLY */
  {ofp_header_trace,}, /* PDUMP_OFP_BARRIER_REQUEST */
  {ofp_header_trace,}, /* PDUMP_OFP_BARRIER_REPLY */
  {ofp_queue_get_config_request_trace,}, /* PDUMP_OFP_QUEUE_GET_CONFIG_REQUEST */
  {ofp_queue_get_config_reply_trace,},  /* PDUMP_OFP_QUEUE_GET_CONFIG_REPLY */
  {ofp_role_request_trace,}, /* PDUMP_OFP_ROLE_REQUEST */
  {ofp_role_request_trace,}, /* PDUMP_OFP_ROLE_REPLY */
  {ofp_header_trace,}, /* PDUMP_OFP_GET_ASYNC_REQUEST */
  {ofp_async_config_trace,}, /* PDUMP_OFP_GET_ASYNC_REPLY */
  {ofp_async_config_trace,}, /* PDUMP_OFP_SET_ASYNC */
  {ofp_meter_mod_trace,}, /* PDUMP_OFP_METER_MOD */
  /* instruction. */
  {ofp_instruction_goto_table_trace,}, /* PDUMP_OFP_INSTRUCTION_GOTO_TABLE */
  {ofp_instruction_write_metadata_trace,}, /* PDUMP_OFP_INSTRUCTION_WRITE_METADATA */
  {ofp_instruction_actions_trace,}, /* PDUMP_OFP_INSTRUCTION_ACTIONS */
  {ofp_instruction_meter_trace,}, /* PDUMP_OFP_INSTRUCTION_METER */
  {ofp_instruction_experimenter_trace,},/* PDUMP_OFP_INSTRUCTION_EXPERIMENTER */
  /* action. */
  {ofp_action_output_trace,}, /* PDUMP_OFP_ACTION_OUTPUT */
  {ofp_action_header_trace,}, /* PDUMP_OFP_ACTION_HEADER */
  {ofp_action_mpls_ttl_trace,},/* PDUMP_OFP_ACTION_MPLS_TTL */
  {ofp_action_push_trace,}, /* PDUMP_OFP_ACTION_PUSH */
  {ofp_action_pop_mpls_trace,}, /* PDUMP_OFP_ACTION_POP_MPLS */
  {ofp_action_set_queue_trace,}, /* PDUMP_OFP_ACTION_SET_QUEUE */
  {ofp_action_group_trace,}, /* PDUMP_OFP_ACTION_GROUP */
  {ofp_action_nw_ttl_trace,}, /* PDUMP_OFP_ACTION_NW_TTL */
  {ofp_action_set_field_trace,},/* PDUMP_OFP_ACTION_SET_FIELD */
  {ofp_action_experimenter_header_trace,}, /* PDUMP_OFP_ACTION_EXPERIMENTER_HEADER */
  /* meter_band. */
  {ofp_meter_band_drop_trace,}, /* PDUMP_OFP_METER_BAND_DROP */
  {ofp_meter_band_dscp_remark_trace,}, /* PDUMP_OFP_METER_BAND_DSCP_REMARK */
  {ofp_meter_band_experimenter_trace,},/* PDUMP_OFP_METER_BAND_EXPERIMENTER */
  /* bucket. */
  {ofp_bucket_trace,}, /* PDUMP_OFP_BUCKET */
  /* match. */
  {match_trace,}, /* PDUMP_OFP_MATCH */
  /* packet_queue */
  {ofp_packet_queue_trace,}, /* PDUMP_OFP_PACKET_QUEUE */
  /* queue_prop */
  {ofp_queue_prop_header_trace,}, /* PDUMP_OFP_QUEUE_PROP_HEADER */
  {ofp_queue_prop_min_rate_trace,}, /* PDUMP_OFP_QUEUE_PROP_MIN_RATE */
  {ofp_queue_prop_max_rate_trace,}, /* PDUMP_OFP_QUEUE_PROP_MAX_RATE */
  {ofp_queue_prop_experimenter_trace,}, /* PDUMP_OFP_QUEUE_PROP_EXPERIMENTER */
  /* flow_stats */
  {ofp_flow_stats_trace,}, /* PDUMP_OFP_FLOW_STATS */
  /* queue_stats */
  {ofp_queue_stats_trace,}, /* PDUMP_OFP_QUEUE_STATS */
  /* bucket_counter */
  {ofp_bucket_counter_trace,}, /* PDUMP_OFP_BUCKET_COUNTER */
  /* group_stats */
  {ofp_group_stats_trace,}, /* PDUMP_OFP_GROUP_STATS */
  /* table_stats */
  {ofp_table_stats_trace,}, /* PDUMP_OFP_TABLE_STATS */
  /* experimenter_data */
  {ofp_experimenter_data_trace,}, /* PDUMP_OFP_EXPERIMENTER_DATA */
  /* oxm_id */
  {ofp_oxm_id_trace,}, /* PDUMP_OFP_OXM_ID */
  /* next_table_id */
  {ofp_next_table_id_trace,}, /* PDUMP_OFP_NEXT_TABLE_ID */
  /* table_features */
  {ofp_table_features_trace,}, /* PDUMP_OFP_TABLE_FEATURES */
  /* table_feature_prop */
  {ofp_table_feature_prop_header_trace,}, /* PDUMP_OFP_TABLE_FEATURE_PROP_HEADER */
  {ofp_table_feature_prop_instructions_trace,}, /* PDUMP_OFP_TABLE_FEATURE_PROP_INSTRUCTIONS */
  {ofp_table_feature_prop_next_tables_trace,}, /* PDUMP_OFP_TABLE_FEATURE_PROP_NEXT_TABLES */
  {ofp_table_feature_prop_actions_trace,}, /* PDUMP_OFP_TABLE_FEATURE_PROP_ACTIONS */
  {ofp_table_feature_prop_oxm_trace,}, /* PDUMP_OFP_TABLE_FEATURE_PROP_OXM */
  {ofp_table_feature_prop_experimenter_trace,}, /* PDUMP_OFP_TABLE_FEATURE_PROP_EXPERIMENTER */
  /* port_stats */
  {ofp_port_stats_trace,}, /* PDUMP_OFP_PORT_STATS */
  /* group_desc */
  {ofp_group_desc_trace,}, /* PDUMP_OFP_GROUP_DESC */
  /* meter_stats */
  {ofp_meter_stats_trace,}, /* PDUMP_OFP_METER_STATS */
  /* meter_band_stats */
  {ofp_meter_band_stats_trace,}, /* PDUMP_OFP_METER_BAND_STATS */
  /* meter_config */
  {ofp_meter_config_trace,}, /* PDUMP_OFP_METER_CONFIG */
  /* port */
  {ofp_port_trace,}, /* PDUMP_OFP_PORT */
};


lagopus_result_t
trace_call(enum trace_pdump_type type,
           const void *pac, size_t pac_size,
           char *str, size_t max_len) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pac != NULL && str != NULL &&
      type < PDUMP_MAX) {
    ret = (*(trace_pdump[type].func))(pac, pac_size, str, max_len);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
