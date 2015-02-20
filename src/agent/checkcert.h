/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#ifndef __CHECKCERT_H__
#define __CHECKCERT_H__





/**
 *      @file   checkcert.h
 */





/**
 * Initialize the certificate checker.
 */
void
agent_check_certificates_initialize(void);


/**
 * Finalize the certficate checker.
 */
void
agent_check_certificates_finalize(void);


/**
 * Set the certificate check configuration file.
 *
 *      @param[in]      filename        A file name of the config.
 */
void
agent_check_certificates_set_config_file(const char *filename);


/**
 * Check a permission of a connection establishment to a peer by DNs.
 *
 *      @param[in]      issuer_dn       A DN of the issuer.
 *      @param[in]      subject_dn      A DN of the subject.
 *
 *      @retval LAGOPUS_RESULT_OK       Succeeded, the permission granted.
 *      @retval LAGOPUS_RESULT_NOT_ALLOWED      Failed, the permission denied.
 *      @retval LAGOPUS_RESULT_INVALID_ARGS     Failed, invalid argument(s).
 *      @retval LAGOPUS_RESULT_NO_MEMORY        Failed, no memory.
 *      @retval LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
agent_check_certificates(const char *issuer_dn, const char *subject_dn);


#endif /* __CHECKCERT_H__ */
