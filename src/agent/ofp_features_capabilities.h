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
 * @file	ofp_features_capabilities.h
 */

#ifndef __OFP_FEATURES_CAPABILITIES_H__
#define __OFP_FEATURES_CAPABILITIES_H__

/**
 * Convert datastore capabilities => features capabilities.
 *
 *     @param[in]	ds_capabilities	capabilities (format to datastore capabilities).
 *     @param[in,out]	features_capabilities	A pointer to features.
 *                                              (format to features capabilities)
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_features_capabilities_convert(uint64_t ds_capabilities,
                                  uint32_t *features_capabilities);

#endif /* __OFP_FEATURES_CAPABILITIES_H__ */
