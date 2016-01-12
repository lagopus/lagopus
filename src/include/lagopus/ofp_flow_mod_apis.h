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
 * @file	ofp_flow_mod_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_flow_mod
 * @details	Describe APIs between Agent and Data-Plane for ofp_flow_mod.
 */
#ifndef __LAGOPUS_OFP_FLOW_MOD_APIS_H__
#define __LAGOPUS_OFP_FLOW_MOD_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/flowdb.h"
#include "lagopus/bridge.h"

/* in flow.c */
/* FlowMod */
/**
 * Add entry to a flow table for \b OFPT_FLOW_MOD(OFPFC_ADD).
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	flow_mod	A pointer to \e flow_mod structure.
 *     @param[in]	match_list	A pointer to list of match structures.
 *     @param[in]	instruction_list	A pointer to list of
 *     instruction structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	Check ofp_flow_mod.flags(OFPFF_SEND_FLOW_REM,
 *     OFPFF_CHECK_OVERLAP, etc.) in this function.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Data-Plane side.
 */
lagopus_result_t
ofp_flow_mod_check_add(uint64_t dpid,
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       struct ofp_error *error);

/**
 * Modify entry in flow tables for \b OFPT_FLOW_MOD(OFPFC_MODIFY*).
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	flow_mod	A pointer to \e flow_mod structure.
 *     @param[in]	match_list	A pointer to list of match structures.
 *     @param[in]	instruction_list	A pointer to list of
 *     instruction structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Data-Plane side.
 */
lagopus_result_t
ofp_flow_mod_modify(uint64_t dpid,
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    struct ofp_error *error);

/**
 * Delete entry in flow tables for \b OFPT_FLOW_MOD(OFPFC_DELETE*).
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	flow_mod	A pointer to \e flow_mod structure.
 *     @param[in]	match_list	A pointer to list of match structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Data-Plane side.
 */
lagopus_result_t
ofp_flow_mod_delete(uint64_t dpid,
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct ofp_error *error);
/* FlowMod END */

#endif /* __LAGOPUS_OFP_FLOW_MOD_APIS_H__ */
