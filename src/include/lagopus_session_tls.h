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

#ifndef __LAGOPUS_SESSION_TLS_H__
#define __LAGOPUS_SESSION_TLS_H__

/**
 * Set a certificate store in a session_tls.
 *
 *  @param[in] new_certstore    A string pointer as the certificate directory.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval LAGOPUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval LAGOPUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval LAGOPUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
lagopus_result_t
lagopus_session_tls_set_ca_dir(const char *c);

/**
 * Set a certificate file for client in a session_tls.
 *
 *  @param[in] new_certfile    A string pointer as the certificate file.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval LAGOPUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval LAGOPUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval LAGOPUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
lagopus_result_t
lagopus_session_tls_set_client_cert(const char *c);

/**
 * Set a certificate file for server in a session_tls.
 *
 *  @param[in] new_certfile    A string pointer as the certificate file.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval LAGOPUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval LAGOPUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval LAGOPUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
lagopus_result_t
lagopus_session_tls_set_server_cert(const char *c);

/**
 * Set a private key for client in a session_tls.
 *
 *  @param[in] new_pvtkey    A string pointer as the private key.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval LAGOPUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval LAGOPUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval LAGOPUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
lagopus_result_t
lagopus_session_tls_set_client_key(const char *c);

/**
 * Set a private key for server in a session_tls.
 *
 *  @param[in] new_pvtkey    A string pointer as the private key.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval LAGOPUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval LAGOPUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval LAGOPUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
lagopus_result_t
lagopus_session_tls_set_server_key(const char *c);

/**
 * Set a trustpoint configuration in a session_tls.
 *
 *  @param[in] new_tpconf    A string pointer as the trustpoint configuration.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval LAGOPUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval LAGOPUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval LAGOPUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
lagopus_result_t
lagopus_session_tls_set_trust_point_conf(const char *c);

/**
 * Get a certificate store in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as certificate directory.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
lagopus_result_t
lagopus_session_tls_get_ca_dir(char **c);

/**
 * Get a certificate file for client in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as certificate file.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
lagopus_result_t
lagopus_session_tls_get_client_cert(char **c);

/**
 * Get a certificate file for server in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as certificate file.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
lagopus_result_t
lagopus_session_tls_get_server_cert(char **c);

/**
 * Get a private key for client in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as private key.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
lagopus_result_t
lagopus_session_tls_get_client_key(char **c);

/**
 * Get a private key for server in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as private key.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
lagopus_result_t
lagopus_session_tls_get_server_key(char **c);

/**
 * Get a trustpoint configuration in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as the trustpoint configuration.
 *
 *  @retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
lagopus_result_t
lagopus_session_tls_get_trust_point_conf(char **c);

/**
 * Set default trust point check function.
 *
 *  @param[in]   s     A session.
 *  @param[in]   func  trust point check function.
 *
 */
void
lagopus_session_tls_set_certcheck_default(lagopus_result_t
    (*func)(const char *, const char *));

/**
 * Set trust point check function to a session.
 *
 *  @param[in]   s     A session.
 *  @param[in]   func  trust point check function for this session.
 *
 */
void
lagopus_session_tls_set_certcheck(lagopus_session_t s, lagopus_result_t
                                  (*func)(const char *, const char *));


#endif /* __LAGOPUS_SESSION_TLS_H__ */
