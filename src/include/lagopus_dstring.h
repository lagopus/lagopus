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
 * @file	lagopus_dstring.h
 */

#ifndef __LAGOPUS_DSTRING_H__
#define __LAGOPUS_DSTRING_H__

/**
 * @brief	lagopus_dstring_t
 */
typedef struct dstring *lagopus_dstring_t;

/**
 * Create a dynamic string.
 *
 *     @param[out]	ds	A pointer to a dynamic string to be created.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
lagopus_dstring_create(lagopus_dstring_t *ds);

/**
 * Destroy a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	void
 */
void
lagopus_dstring_destroy(lagopus_dstring_t *ds);

/**
 * Clear a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
lagopus_result_t
lagopus_dstring_clear(lagopus_dstring_t *ds);

/**
 * Append a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
lagopus_dstring_vappendf(lagopus_dstring_t *ds, const char *format,
                         va_list *args) __attr_format_printf__(2, 0);

/**
 * Append a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
lagopus_dstring_appendf(lagopus_dstring_t *ds, const char *format, ...)
__attr_format_printf__(2, 3);

/**
 * Prepend a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
lagopus_dstring_vprependf(lagopus_dstring_t *ds, const char *format,
                          va_list *args) __attr_format_printf__(2, 0);

/**
 * repend a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
lagopus_dstring_prependf(lagopus_dstring_t *ds, const char *format, ...)
    __attr_format_printf__(2, 3);

/**
 * Insert a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	offset	offset.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
lagopus_dstring_vinsertf(lagopus_dstring_t *ds,
                         size_t offset,
                         const char *format,
                         va_list *args) __attr_format_printf__(3, 0);

/**
 * Prepend a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	offset	offset.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
lagopus_dstring_insertf(lagopus_dstring_t *ds,
                        size_t offset,
                        const char *format, ...)
    __attr_format_printf__(3, 4);


/**
 * Concatenate dynamic strings.
 *
 *     @param[in,out]	dst_ds	A pointer to a dynamic string(dst).
 *     @param[in,out]	src_ds	A pointer to a dynamic string(src).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
lagopus_dstring_concat(lagopus_dstring_t *dst_ds,
                       const lagopus_dstring_t *src_ds);

/**
 * Get string from a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[out]	ds	A pointer to a string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
lagopus_result_t
lagopus_dstring_str_get(lagopus_dstring_t *ds, char **str);

/**
 * Get size of a string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	>=0	Size of a string
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
lagopus_result_t
lagopus_dstring_len_get(lagopus_dstring_t *ds);

/**
 * A dynamic string is empty.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	true/false
 */
bool
lagopus_dstring_empty(lagopus_dstring_t *ds);

#endif /* __LAGOPUS_DSTRING_H__ */
