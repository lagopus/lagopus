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
 * @file	ofp_table_mod_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_table_mod
 * @details	Describe APIs between Agent and Data-Plane for ofp_table_mod.
 */
#ifndef __LAGOPUS_OFP_TABLE_MOD_APIS_H__
#define __LAGOPUS_OFP_TABLE_MOD_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/bridge.h"


/* TableMod */
/**
 * The table configuration which match \e table_id is updated for \b OFP_TABLE_MOD.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	table_mod	A pointer to \e ofp_table_mod structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_table_mod_set(uint64_t dpid,
                  struct ofp_table_mod *table_mod,
                  struct ofp_error *error);
/* TableMod END */

#endif /* __LAGOPUS_OFP_TABLE_MOD_APIS_H__ */
