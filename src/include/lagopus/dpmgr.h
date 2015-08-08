/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
 * @file        dpmgr.h
 * @brief       Datapath manager.
 */

#ifndef SRC_INCLUDE_LAGOPUS_DPMGR_H_
#define SRC_INCLUDE_LAGOPUS_DPMGR_H_

#include "lagopus_apis.h"
#include "bridge.h"

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
dpmgr_switch_mode_get(uint64_t dpid, enum switch_mode *switch_mode) __attribute__ ((deprecated));

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
dpmgr_switch_mode_set(uint64_t dpid, enum switch_mode switch_mode) __attribute__ ((deprecated));

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
dpmgr_bridge_fail_mode_get(uint64_t dpid,
                           enum fail_mode *fail_mode) __attribute__ ((deprecated));

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
dpmgr_bridge_ofp_version_get(uint64_t dpid, uint8_t *version) __attribute__ ((deprecated));

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
dpmgr_bridge_ofp_version_set(uint64_t dpid, uint8_t version) __attribute__ ((deprecated));

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
dpmgr_bridge_ofp_version_bitmap_get(uint64_t dpid,
                                    uint32_t *version_bitmap) __attribute__ ((deprecated));

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
dpmgr_bridge_ofp_version_bitmap_set(uint64_t dpid, uint8_t version) __attribute__ ((deprecated));

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
dpmgr_bridge_ofp_features_get(uint64_t dpid,
                              struct ofp_switch_features *features) __attribute__ ((deprecated));

#endif /* SRC_INCLUDE_LAGOPUS_DPMGR_H_ */
