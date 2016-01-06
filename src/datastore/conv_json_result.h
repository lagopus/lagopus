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
 * @file	conv_json_result.h
 */

#ifndef __CONV_JSON_RESULT_H__
#define __CONV_JSON_RESULT_H__

struct datastore_json_result;

/**
 * Get result string for json.
 *
 *     @param[in]	ret	Result code.
 *
 *     @retval	Result string.
 */
const char *
datastore_json_result_string_get(lagopus_result_t ret);

#endif /* __CONV_JSON_RESULT_H__ */
