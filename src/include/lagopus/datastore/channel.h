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

#ifndef __LAGOPUS_DATASTORE_CHANNEL_H__
#define __LAGOPUS_DATASTORE_CHANNEL_H__

#include "lagopus_ip_addr.h"

typedef enum datastore_channel_protocol {
  DATASTORE_CHANNEL_PROTOCOL_UNKNOWN = 0,
  DATASTORE_CHANNEL_PROTOCOL_TCP,
  DATASTORE_CHANNEL_PROTOCOL_TLS,
  DATASTORE_CHANNEL_PROTOCOL_MIN = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN,
  DATASTORE_CHANNEL_PROTOCOL_MAX = DATASTORE_CHANNEL_PROTOCOL_TLS,
} datastore_channel_protocol_t;

typedef enum datastore_channel_status {
  DATASTORE_CHANNEL_DISONNECTED = 0,
  DATASTORE_CHANNEL_CONNECTED,
} datastore_channel_status_t;

/**
 * Get the value to attribute 'enabled' of the channel table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_channel_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the channel table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_channel_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'dst_addr' of the channel table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dst_addr the value of attribute 'dst_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dst_addr' getted sucessfully.
 */
lagopus_result_t
datastore_channel_get_dst_addr(const char *name, bool current,
                               lagopus_ip_address_t **dst_addr);

/**
 * Get the value to attribute 'dst_addr' of the channel table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dst_addr the value of attribute 'dst_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dst_addr' getted sucessfully.
 */
lagopus_result_t
datastore_channel_get_dst_addr_str(const char *name, bool current,
                                   char **dst_addr);

/**
 * Get the value to attribute 'dst_port' of the channel table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dst_port the value of attribute 'dst_port'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dst_port' getted sucessfully.
 */
lagopus_result_t
datastore_channel_get_dst_port(const char *name, bool current,
                               uint16_t *dst_port);

/**
 * Get the value to attribute 'local_addr' of the channel table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] local_addr the value of attribute 'local_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'local_addr' getted sucessfully.
 */
lagopus_result_t
datastore_channel_get_local_addr(const char *name, bool current,
                                 lagopus_ip_address_t **local_addr);

/**
 * Get the value to attribute 'local_addr' of the channel table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] local_addr the value of attribute 'local_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'local_addr' getted sucessfully.
 */
lagopus_result_t
datastore_channel_get_local_addr_str(const char *name, bool current,
                                     char **local_addr);

/**
 * Get the value to attribute 'local_port' of the channel table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] local_port the value of attribute 'local_port'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'local_port' getted sucessfully.
 */
lagopus_result_t
datastore_channel_get_local_port(const char *name, bool current,
                                 uint16_t *local_port);

/**
 * Get the value to attribute 'protocol' of the channel table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] protocol the value of attribute 'protocol'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'protocol' getted sucessfully.
 */
lagopus_result_t
datastore_channel_get_protocol(const char *name, bool current,
                               datastore_channel_protocol_t *protocol);

#endif /* ! __LAGOPUS_DATASTORE_CHANNEL_H__ */

