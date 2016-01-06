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
 * @file	conv_json.h
 */

#ifndef __CONV_JSON_H__
#define __CONV_JSON_H__

#include "conv_json_result.h"

#define KEY_FMT "\"%s\":"
#define DELIMITER ","
#define DELIMITER_INSTERN(_fmt) (DELIMITER "\n" _fmt)
#define DELIMITER_INSTER(_fmt) (DELIMITER _fmt)

#define DSTRING_CHECK_APPENDF(_ds, _delim,_fmt, ...)                    \
  (((_delim) == true) ?                                                 \
   lagopus_dstring_appendf((_ds), DELIMITER_INSTERN(_fmt),              \
                           ##__VA_ARGS__) :                             \
   lagopus_dstring_appendf((_ds), (_fmt), ##__VA_ARGS__))

#define DS_JSON_LIST_ITEM_FMT(_i)                                       \
  (((_i) == 0) ? "\"%s\"" : DELIMITER_INSTER("\"%s\""))

#define DS_JSON_DELIMITER(_is_first, _fmt)                              \
  (((_is_first) == true) ?  (_fmt) : DELIMITER_INSTERN(_fmt))

#define CMD_ERR_MSG_INVALID_OPT			"opt = %s"
#define CMD_ERR_MSG_INVALID_OPT_VALUE		"Bad opt value = %s"
#define CMD_ERR_MSG_UNREADABLE			"%s is unreadable."
#define CMD_ERR_MSG_UNWRITABLE			"%s is unwritable."
#define CMD_ERR_MSG_LOAD_FAILED			"Load Failed(%s)"
#define CMD_ERR_MSG_SAVE_FAILED			"Save Failed(%s)"

/**
 * Set result for JSON string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	result_code	Result code
 *     @param[in]	val	A string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
#define datastore_json_result_string_setf(_ds, _result_code, _msg, ...) \
  datastore_json_result_setf_with_log(_ds, _result_code, true,          \
                                      __FILE__, __LINE__, __PROC__,     \
                                      _msg, ##__VA_ARGS__)
/**
 * Set result.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	result_code	Result code
 *     @param[in]	val	A string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
#define datastore_json_result_set(_ds, _result_code, _val)              \
  datastore_json_result_set_with_log(_ds, _result_code,                 \
                                     __FILE__, __LINE__, __PROC__,      \
                                     _val)

/**
 * Set result according to a format.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	result_code	Result code
 *     @param[in]	format	A pointer to a format.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
#define datastore_json_result_setf(_ds, _result_code, ...)              \
  datastore_json_result_setf_with_log(_ds, _result_code, false,         \
                                      __FILE__, __LINE__, __PROC__,     \
                                      __VA_ARGS__)

/**
 * Set result with output log.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	result_code	Result code
 *     @param[in]	file	File name.
 *     @param[in]	line	Line no.
 *     @param[in]	func	Func name.
 *     @param[in]	val	A string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_result_set_with_log(lagopus_dstring_t *ds,
                                   lagopus_result_t result_code,
                                   const char *file,
                                   int line,
                                   const char *func,
                                   const char *val);

/**
 * Set result according to a format with output log.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	result_code	Result code
 *     @param[in]	is_json_string	Json string flag.
 *     @param[in]	file	File name.
 *     @param[in]	line	Line no.
 *     @param[in]	func	Func name.
 *     @param[in]	format	A pointer to a format.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_result_setf_with_log(lagopus_dstring_t *ds,
                                    lagopus_result_t result_code,
                                    bool is_json_string,
                                    const char *file,
                                    int line,
                                    const char *func,
                                    const char *format, ...)
__attr_format_printf__(7, 8);

/**
 * Convert to JSON from String.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	val	value string.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_string_append(lagopus_dstring_t *ds,
                             const char *key, const char *val,
                             bool delimiter);

/**
 * Convert to JSON from uint64.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	val	int.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_uint64_append(lagopus_dstring_t *ds,
                             const char *key, const uint64_t val,
                             bool delimiter);

/**
 * Convert to JSON from uint32.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	val	int.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_uint32_append(lagopus_dstring_t *ds,
                             const char *key, const uint32_t val,
                             bool delimiter);

/**
 * Convert to JSON from uint16.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	val	int.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_uint16_append(lagopus_dstring_t *ds,
                             const char *key, const uint16_t val,
                             bool delimiter);

/**
 * Convert to JSON from uint8.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	val	int.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_uint8_append(lagopus_dstring_t *ds,
                            const char *key, const uint8_t val,
                            bool delimiter);

/**
 * Convert to JSON from bool.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	val	bool.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_bool_append(lagopus_dstring_t *ds,
                           const char *key, const bool val, bool delimiter);

/**
 * Convert to JSON from string array.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	val	A pointer to a string array.
 *     @param[in]	size	Size of a string array.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_string_array_append(lagopus_dstring_t *ds,
                                   const char *key, const char *val[],
                                   const size_t size,
                                   bool delimiter);

/**
 * Escape string (" and ').
 *
 *     @param[in]	in_str	A input string.
 *     @param[out]	out_str	A output string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_string_escape(const char *in_str, char **out_str);


/**
 * Convert to JSON from flags (uint32 => string).
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	flags	flags
 *     @param[in]	strs	flag strings.
 *     @param[in]	strs	flag strings size.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_uint32_flags_append(lagopus_dstring_t *ds,
                                   const char *key,
                                   uint32_t flags,
                                   const char *const strs[],
                                   size_t strs_size,
                                   bool delimiter);

/**
 * Convert to JSON from flags (uint16 => string).
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[in]	key	key string.
 *     @param[in]	flags	flags
 *     @param[in]	strs	flag strings.
 *     @param[in]	strs	flag strings size.
 *     @param[in]	delimiter	Insert delimiter flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
datastore_json_uint16_flags_append(lagopus_dstring_t *ds,
                                   const char *key,
                                   uint16_t flags,
                                   const char *const strs[],
                                   size_t strs_size,
                                   bool delimiter);

#endif /* __CONV_JSON_H__ */
