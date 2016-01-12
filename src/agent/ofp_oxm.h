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
 * @file	ofp_oxm.h
 */

#ifndef __OFP_OXM_H__
#define __OFP_OXM_H__

#define OXM_FIELD_TYPE(X)   ((X) >> 1)

/**
 * Check oxm.
 *
 *     @param[in]	oxm_field	oxm field.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OXM_ERROR Failed, Bad oxm.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_oxm_field_check(uint8_t oxm_field);

#endif /* __OFP_OXM__H__ */
