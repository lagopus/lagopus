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
 * @file	lagopus_ip_addr.h
 */

#ifndef __LAGOPUS_IP_ADDR_H__
#define __LAGOPUS_IP_ADDR_H__

#define LAGOPUS_ADDR_STR_MAX NI_MAXHOST

/**
 * @brief	lagopus_ip_address_t
 */
typedef struct ip_address lagopus_ip_address_t;

/**
 * Create a lagopus_ip_address_t.
 *
 *     @param[in]	name	Host name.
 *     @param[in]	is_ipv4_addr	IPv4 flag.
 *     @param[out]	ip	A pointer to a \e lagopus_ip_address_t structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
lagopus_ip_address_create(const char *name, bool is_ipv4_addr,
                          lagopus_ip_address_t **ip);

/**
 * Destroy a lagopus_ip_address_t
 *
 *     @param[in]	ip	A pointer to a \e lagopus_ip_address_t structure.
 *
 *     @retval	void
 */
void
lagopus_ip_address_destroy(lagopus_ip_address_t *ip);

/**
 * Copy a lagopus_ip_address_t.
 *
 *     @param[in]	name	Host name.
 *     @param[in]	src	A pointer to a \e lagopus_ip_address_t structure (src).
 *     @param[out]	dst	A pointer to a \e lagopus_ip_address_t structure (dst).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
lagopus_ip_address_copy(const lagopus_ip_address_t *src,
                        lagopus_ip_address_t **dst);

/**
 * Equals a lagopus_ip_address_t.
 *
 *     @param[in]	ip1	A pointer to a \e lagopus_ip_address_t structure (src).
 *     @param[in]	ip2	A pointer to a \e lagopus_ip_address_t structure (dst).
 *
 *     @retval  true/false
 */
bool
lagopus_ip_address_equals(const lagopus_ip_address_t *ip1,
                          const lagopus_ip_address_t *ip2);

/**
 * Get IP addr string.
 *
 *     @param[in]	ip	A pointer to a \e lagopus_ip_address_t structure.
 *     @param[out]	addr_str	IP addr string..
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
lagopus_ip_address_str_get(const lagopus_ip_address_t *ip,
                           char **addr_str);

/**
 * Get sockaddr structure.
 *
 *     @param[in]	ip	A pointer to a \e lagopus_ip_address_t structure.
 *     @param[out]	saddr	A pointer to a \e sockaddr structure
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
lagopus_ip_address_sockaddr_get(const lagopus_ip_address_t *ip,
                                struct sockaddr **saddr);

/**
 * Get length of sockaddr structure.
 *
 *     @param[in]	ip	A pointer to a \e lagopus_ip_address_t structure.
 *     @param[out]	saddr_len	A pointer to a \e length of sockaddr structure
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
lagopus_result_t
lagopus_ip_address_sockaddr_len_get(const lagopus_ip_address_t *ip,
                                    socklen_t *saddr_len);


/**
 * Is IPv4.
 *
 *     @param[in]	ip	A pointer to a \e lagopus_ip_address_t structure.
 *     @param[out]	is_ipv4	A pointer to a \e is_ipv4.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
lagopus_result_t
lagopus_ip_address_is_ipv4(const lagopus_ip_address_t *ip,
                           bool *is_ipv4);

#endif /* __LAGOPUS_IP_ADDR_H__ */
