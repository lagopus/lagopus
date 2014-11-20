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
 * @file        dpmgr.h
 * @brief       Datapath manager.
 */

#ifndef SRC_INCLUDE_LAGOPUS_DPMGR_H_
#define SRC_INCLUDE_LAGOPUS_DPMGR_H_

#include "lagopus_apis.h"
#include "bridge.h"

struct vector;
struct port;

/**
 * Datapath manager object.
 * All ports and all bridges are managed by single manager object.
 */
struct dpmgr {
  struct vector *ports;                 /*< Physical ports. */
  struct bridge_list bridge_list;       /*< Bridges. */
};

/**
 * Generate datapath id value.
 *
 * @retval      64bit randomized value used for datapath id.
 */
uint64_t
dpmgr_dpid_generate(void);

/**
 * Allocate datapath manager object.
 *
 * @retval      !=NULL  datapath manager object.
 * @retval      ==NULL  Memory exhausted.
 */
struct dpmgr *
dpmgr_alloc(void);

/**
 * Free datapath manager object, includes all port and bridge objects.
 *
 * @param[in]   dpmgr   datapath manager object.
 */
void
dpmgr_free(struct dpmgr *dmpgr);

/**
 * Get allocated dpmgr instance.
 *
 * @retval      !=NULL  dpmgr instance.
 * @retval      ==NULL  dpmgr is not allocated.
 */
struct dpmgr *dpmgr_get_instance(void);

/**
 * Create and register port to datapath manager.
 *
 * @param[in]   dpgmr           Datapath manager.
 * @param[in]   port_param      Parameters of the port.
 *
 * @retval      LAGOPUS_RESULT_OK       Succeeded.
 * @retval      !=LAGOPUS_RESULT_OK     failed.
 */
lagopus_result_t
dpmgr_port_add(struct dpmgr *dpmgr, const struct port *port_param);

/**
 * Unregister port from datapath manager and delete port.
 *
 * @param[in]   dpgmr           Datapath manager.
 * @param[in]   port_ifindex    Physical port index.
 *
 */
lagopus_result_t
dpmgr_port_delete(struct dpmgr *dpmgr, uint32_t port_ifindex);

/**
 * Create and register bridge (a.k.a. OpenFlow Switch.)
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   bridge_name     Bridge name.
 * @param[in]   dpid            Datapath id associated with the bridge.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_ALREADY_EXISTS   Already exist same name bridge.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
lagopus_result_t
dpmgr_bridge_add(struct dpmgr *dpmgr, const char *bridge_name, uint64_t dpid);

/**
 * Delete bridge.
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   bridge_name     Bridge name.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not exist.
 */
lagopus_result_t
dpmgr_bridge_delete(struct dpmgr *dpmgr, const char *bridge_name);

/**
 * Lookup bridge.
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   bridge_name     Bridge name.
 *
 * @retval      !=NULL          Bridge object.
 * @retval      ==NULL          Bridge asscoated with name is not exist.
 */
struct bridge *
dpmgr_bridge_lookup(struct dpmgr *dpmgr, const char *bridge_name);

/**
 * Lookup bridge by controller address.
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   address         Controller address.
 *
 * @retval      !=NULL          Bridge object.
 * @retval      ==NULL          Bridge asscoated with name is not exist.
 */
struct bridge *
dpmgr_bridge_lookup_by_controller_address(struct dpmgr *dpmgr,
    const char *controller_address);
/**
 * Lookup bridge dpid.
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   bridge_name     Bridge name.
 * @param[out]  dpidp           Placeholder of datapath id.
 *
 * @retval      LAGOPUS_RESULT_OK               Success.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not found.
 */
lagopus_result_t
dpmgr_bridge_dpid_lookup(struct dpmgr *dpmgr,
                         const char *bridge_name,
                         uint64_t *dpidp);

/**
 * Set/update bridge dpid.
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   bridge_name     Bridge name.
 * @param[in]   dpid            Datapath id.
 *
 * @retval      LAGOPUS_RESULT_OK               Success.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not found.
 */
lagopus_result_t
dpmgr_bridge_dpid_set(struct dpmgr *dpmgr,
                      const char *bridge_name,
                      uint64_t dpid);

/**
 * Assign port to specified bridge.
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   bridge_name     Bridge name.
 * @param[in]   portid          Physical ifindex.
 * @param[in]   port_no         Assigned OpenFlow port number.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_ALREADY_EXISTS   Port is alrady assinged.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge or port is not exist.
 */
lagopus_result_t
dpmgr_bridge_port_add(struct dpmgr *dpmgr, const char *bridge_name,
                      uint32_t portid, uint32_t port_no);

/**
 * Resign port to specified bridge.
 *
 * @param[in]   dpmgr           Datapath manager.
 * @param[in]   bridge_name     Bridge name.
 * @param[in]   port_ifindex    Physical port index.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge or port is not exist.
 */
lagopus_result_t
dpmgr_bridge_port_delete(struct dpmgr *dpmgr, const char *bridge_name,
                         uint32_t port_ifindex);

/**
 * Add controller address to the bridge.
 *
 * @param[in]   dpmgr                   Datapath manager.
 * @param[in]   bridge_name             Bridge name.
 * @param[in]   controller_addresss     Controller address string.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 * @retval      LAGOPUS_RESULT_NOT_FOUND Bridge is not exist.
 */
lagopus_result_t
dpmgr_controller_add(struct dpmgr *dpmgr, const char *bridge_name,
                     const char *controller_address);

/**
 * Delete controller address to the bridge.
 *
 * @param[in]   dpmgr                   Datapath manager.
 * @param[in]   bridge_name             Bridge name.
 * @param[in]   controller_addresss     Controller address string.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 * @retval      LAGOPUS_RESULT_NOT_FOUND Bridge is not exist.
 */
lagopus_result_t
dpmgr_controller_delete(struct dpmgr *dpmgr, const char *bridge_name,
                        const char *controller_address);

/**
 */
lagopus_result_t
dpmgr_controller_dpid_find(struct dpmgr *, const char *, uint64_t *);

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
dpmgr_switch_mode_get(uint64_t dpid, enum switch_mode *switch_mode);

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
dpmgr_switch_mode_set(uint64_t dpid, enum switch_mode switch_mode);

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
dpmgr_bridge_ofp_version_get(uint64_t dpid, uint8_t *version);

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
dpmgr_bridge_ofp_version_set(uint64_t dpid, uint8_t version);

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
dpmgr_bridge_ofp_version_bitmap_set(uint64_t dpid, uint8_t version);

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
                              struct ofp_switch_features *features);

#endif /* SRC_INCLUDE_LAGOPUS_DPMGR_H_ */
