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
 * @file	ofp_meter_mod_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_meter_mod
 * @details	Describe APIs between Agent and Data-Plane for ofp_meter_mod.
 */
#ifndef __LAGOPUS_OFP_METER_MOD_APIS_H__
#define __LAGOPUS_OFP_METER_MOD_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/meter.h"
#include "lagopus/bridge.h"

/* MeterMod */
/**
 * Add entry to a meter table for \b OFPT_METER_MOD(OFPMC_ADD).
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	meter_mod	A pointer to \e meter_mod structure.
 *     @param[in]	band_list	A pointer to list of meter bands.
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
ofp_meter_mod_add(uint64_t dpid,
                  struct ofp_meter_mod *meter_mod,
                  struct meter_band_list *band_list,
                  struct ofp_error *error);
/**
 * A meter table which match \e meter_id is updated
 * for \b OFPT_METER_MOD(OFPMC_MODIFY).
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	meter_mod	A pointer to \e meter_mod structure.
 *     @param[in]	band_list	A pointer to list of meter bands.
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
ofp_meter_mod_modify(uint64_t dpid,
                     struct ofp_meter_mod *meter_mod,
                     struct meter_band_list *band_list,
                     struct ofp_error *error);

/**
 * A meter table which match \e meter_id is deleted
 * for \b OFPT_METER_MOD(OFPMC_MODIFY).
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	meter_mod	A pointer to \e meter_mod structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_meter_mod_delete(uint64_t dpid,
                     struct ofp_meter_mod *meter_mod,
                     struct ofp_error *error);

/* MeterMod END */

#endif /* __LAGOPUS_OFP_METER_MOD_APIS_H__ */
