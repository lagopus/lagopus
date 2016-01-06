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
 * @file	ofp_switch_config_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_switch_config
 * @details	Describe APIs between Agent and Data-Plane for ofp_switch_config.
 */
#ifndef __LAGOPUS_OFP_SWITCH_CONFIG_APIS_H__
#define __LAGOPUS_OFP_SWITCH_CONFIG_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/bridge.h"


/* Switch Config */
/**
 * Update the switch configuration for \b OFPT_SET_CONFIG.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	switch_config	A pointer to \e ofp_switch_config structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_switch_config_set(uint64_t dpid,
                      struct ofp_switch_config *switch_config,
                      struct ofp_error *error);

/**
 * Get switch configuration.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	switch_config	A pointer to \e ofp_switch_config structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_switch_config_get(uint64_t dpid,
                      struct ofp_switch_config *switch_config,
                      struct ofp_error *error);
/* Switch Config END */

/**
 * Get current switch mode (openflow, secure or standalone).
 *
 * @param[in]   dpid    Datapath ID.
 * @param[out]  switch_mode     Current switch_mode.
 *
 * @retval      LAGOPUS_RESULT_OK       Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_mode_get(uint64_t dpid, enum switch_mode *switch_mode);

/**
 * Set switch mode (openflow, secure or standalone).
 *
 * @param[in]   dpid    Datapath ID.
 * @param[in]   switch_mode     switch_mode to be set.
 *
 * @retval      LAGOPUS_RESULT_OK       Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_mode_set(uint64_t dpid, enum switch_mode switch_mode);

/**
 * Get OpenFlow switch fail mode.
 *
 * @param[in]   dpid    Datapath ID.
 * @param[out]  fail_mode       Bridge fail mode.
 *
 * @retval      LAGOPUS_RESULT_OK       Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_fail_mode_get(uint64_t dpid,
                         enum fail_mode *fail_mode);

/**
 * Get primary OpenFlow version.
 *
 * @param[in]   dpid    Datapath ID.
 * @param[out]  version Wire protocol version.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_version_get(uint64_t dpid, uint8_t *version);

/**
 * Set primary OpenFlow version.
 *
 * @param[in]   dpid    Datapath ID.
 * @param[in]   version Wire protocol version.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_INVALID_ARGS     Version number is wrong.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_version_set(uint64_t dpid, uint8_t version);

/**
 * Get supported OpenFlow version bitmap.
 *
 * @param[in]   dpid    Datapath ID.
 * @param[out]  version_bitmap  Support version bitmap.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_version_bitmap_get(uint64_t dpid,
                              uint32_t *version_bitmap);

/**
 * Set supported OpenFlow version bitmap.
 *
 * @param[in]   dpid    Datapath ID.
 * @param[in]   version Wire protocol version to be set to version bitmap.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_INVALID_ARGS     Version number is wrong.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_version_bitmap_set(uint64_t dpid, uint8_t version);

/**
 * Get OpenFlow switch features.
 *
 * @param[in]   dpid    Datapath ID.
 * @param[out]  features        ofp_switch_features structure.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Failed, Not found.
 * @retval      LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_switch_features_get(uint64_t dpid,
                        struct ofp_switch_features *features);

#endif /* __LAGOPUS_OFP_SWITCH_CONFIG_APIS_H__ */
