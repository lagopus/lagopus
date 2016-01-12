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
 * @file	ofp_meter.h
 */

#ifndef __OFP_METER_H__
#define __OFP_METER_H__

/**
 * Check meter id.
 *
 *     @param[in]	meter_id 	meter id.
 *     @param[out]	error 	A pointer to \e ofp_error structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OXM_ERROR Failed, Invalid meter_id.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_meter_id_check(uint32_t meter_id, struct ofp_error *error);

#endif /* __OFP_METER_H__ */
