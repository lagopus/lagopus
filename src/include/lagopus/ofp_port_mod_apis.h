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
 * @file	ofp_port_mod_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_port_mod
 * @details	Describe APIs between Agent and Data-Plane for ofp_port_mod.
 */
#ifndef __LAGOPUS_OFP_PORT_MOD_APIS_H__
#define __LAGOPUS_OFP_PORT_MOD_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/bridge.h"

/* PortMod */
/**
 * The port configuration which match \e port_id is updated
 * for \b OFPT_PORT_MOD.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	port_mod	A pointer to \e port_mod structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_port_mod_modify(uint64_t dpid,
                    struct ofp_port_mod *port_mod,
                    struct ofp_error *error);
/* PortMod END */

#endif /* __LAGOPUS_OFP_PORT_MOD_APIS_H__ */
