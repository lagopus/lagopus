/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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


#ifndef __SESSION_TLS_H__
#define __SESSION_TLS_H__

void
session_tls_initialize(lagopus_result_t
                       (*func)(const char *, const char *));
void
session_tls_ca_dir_set(const char *c);
void
session_tls_ca_dir_get(char **c);
void
session_tls_cert_set(const char *c);
void
session_tls_cert_get(char **c);
void
session_tls_key_set(const char *c);
void
session_tls_key_get(char **c);
void
session_tls_certcheck_set(lagopus_result_t
                          (*func)(const char *, const char *));
#endif /* __SESSION_TLS_H__ */
