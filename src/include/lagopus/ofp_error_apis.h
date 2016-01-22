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
 * @file	ofp_error_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_error.
 * @details	Describe APIs between Agent and Data-Plane for ofp_error.
 */
#ifndef __LAGOPUS_OFP_ERROR_APIS_H__
#define __LAGOPUS_OFP_ERROR_APIS_H__

/**
 *  Maximum size of error string.
 */
#define ERROR_MAX_SIZE 4096

/**
 * Set ofp_error.
 *
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     @param[in]	type	Type of error.
 *     @param[in]	code	Code of error.
 */
#define ofp_error_set(__error, __type, __code) {        \
    char __ebuf[ERROR_MAX_SIZE];                        \
    /* set error. */                                    \
    ofp_error_val_set((__error), __type, __code);       \
    /* put log. */                                      \
    ofp_error_str_get(__type, __code,                   \
                      __ebuf, ERROR_MAX_SIZE);          \
    lagopus_msg_warning("%s\n", __ebuf);                \
  }

/**
 * Set ofp_error values.
 *
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     @param[in]	type	Type of error.
 *     @param[in]	code	Code of error.
 */
void
ofp_error_val_set(struct ofp_error *error, uint16_t type, uint16_t code);

/**
 * Get error(\e ofp_error) string.
 *
 *     @param[in]	type	Type of error.
 *     @param[in]	code	Code of error.
 *     @param[out]	string	A pointer to error string.
 *     @param[in]	max_len	Maximum size of error string.
 *
 *     @retval	void
 */
void
ofp_error_str_get(uint16_t type, uint16_t code,
                  char *str, size_t max_len);

#endif /* __LAGOPUS_OFP_ERROR_APIS_H__ */
