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

#ifndef __LAGOPUS_STRUTILS_H__
#define __LAGOPUS_STRUTILS_H__





/**
 *	@file	lagopus_strutils.h
 */





lagopus_result_t
lagopus_str_tokenize_with_limit(char *buf, char **tokens,
                                size_t max, size_t limit, const char *delm);

#define lagopus_str_tokenize(_buf, _tokens, _max, _delm) \
  lagopus_str_tokenize_with_limit((_buf), (_tokens), (_max), 0, (_delm))

lagopus_result_t
lagopus_str_tokenize_quote(char *buf, char **tokens,
                           size_t max, const char *delm, const char *quote);

lagopus_result_t
lagopus_str_unescape(const char *org, const char *escaped,
                     char **retptr);

lagopus_result_t
lagopus_str_escape(const char *in_str, const char *escape_chars,
                   bool *is_escaped, char **out_str);

lagopus_result_t
lagopus_str_trim_right(const char *org, const char *trim_chars,
                       char **retptr);

lagopus_result_t
lagopus_str_trim_left(const char *org, const char *trimchars,
                      char **retptr);

lagopus_result_t
lagopus_str_trim(const char *org, const char *trimchars,
                 char **retptr);





lagopus_result_t
lagopus_str_parse_int16_by_base(const char *buf, int16_t *val,
                                unsigned int base);
lagopus_result_t
lagopus_str_parse_int16(const char *buf, int16_t *val);
lagopus_result_t
lagopus_str_parse_uint16_by_base(const char *buf, uint16_t *val,
                                 unsigned int base);
lagopus_result_t
lagopus_str_parse_uint16(const char *buf, uint16_t *val);





lagopus_result_t
lagopus_str_parse_int32_by_base(const char *buf, int32_t *val,
                                unsigned int base);
lagopus_result_t
lagopus_str_parse_int32(const char *buf, int32_t *val);
lagopus_result_t
lagopus_str_parse_uint32_by_base(const char *buf, uint32_t *val,
                                 unsigned int base);
lagopus_result_t
lagopus_str_parse_uint32(const char *buf, uint32_t *val);





lagopus_result_t
lagopus_str_parse_int64_by_base(const char *buf, int64_t *val,
                                unsigned int base);
lagopus_result_t
lagopus_str_parse_int64(const char *buf, int64_t *val);
lagopus_result_t
lagopus_str_parse_uint64_by_base(const char *buf, uint64_t *val,
                                 unsigned int base);
lagopus_result_t
lagopus_str_parse_uint64(const char *buf, uint64_t *val);





lagopus_result_t
lagopus_str_parse_float(const char *buf, float *val);


lagopus_result_t
lagopus_str_parse_double(const char *buf, double *val);


lagopus_result_t
lagopus_str_parse_long_double(const char *buf, long double *val);


lagopus_result_t
lagopus_str_parse_bool(const char *buf, bool *val);





lagopus_result_t
lagopus_str_indexof(const char *str1, const char *str2);


#endif /* ! __LAGOPUS_STRUTILS_H__ */
