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
 * @file	cmd_dump.h
 */

#ifndef __CMD_DUMP_H__
#define __CMD_DUMP_H__

#include "conv_json.h"
#include "conv_json_result.h"
#include "datastore_internal.h"

typedef lagopus_result_t
(*cmd_dump_proc_t)(datastore_interp_t *iptr,
                   void *c,
                   FILE *fp,
                   void *stream_out,
                   datastore_printf_proc_t printf_proc,
                   bool is_with_stats,
                   lagopus_dstring_t *result);

/**
 * Write dump file.
 *
 *     @param[in]	fp 	A pointer to a file.
 *     @param[in]	proc	An sub command parser proc.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
cmd_dump_file_write(FILE *fp,
                    lagopus_dstring_t *result);

/**
 * Send dump file.
 *
 *     @param[in]	iptr	An interpreter.
 *     @param[in]	fp 	A pointer to a file.
 *     @param[in]	stream_out	A pointer to a out stream.
 *     @param[in]	printf_proc	A pointer to a print func for out stream
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
cmd_dump_file_send(datastore_interp_t *iptr,
                   FILE *fp,
                   void *stream_out,
                   datastore_printf_proc_t printf_proc,
                   lagopus_dstring_t *result);

/**
 * Send error.
 *
 *     @param[in]	stream_out	A pointer to a out stream.
 *     @param[in]	printf_proc	A pointer to a print func for out stream
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
cmd_dump_error_send(void *stream_out,
                    datastore_printf_proc_t printf_proc,
                    lagopus_dstring_t *result);

/**
 * Dump main.
 *
 *     @param[in]	thd	A pointer to a this thread.
 *     @param[in]	iptr	An interpreter.
 *     @param[in]	conf 	A pointer to a conf.
 *     @param[in]	stream_out	A pointer to a out stream.
 *     @param[in]	printf_proc	A pointer to a print func for out stream
 *     @param[in]	ftype	Type of lagopus config file.
 *     @param[in]	file_name	Dump file.
 *     @param[in]	is_with_stats	Dump with stats.
 *     @param[in]	dump_proc	Dump func.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	SLAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
cmd_dump_main(lagopus_thread_t *thd,
              datastore_interp_t *iptr,
              void *conf,
              void *stream_out,
              datastore_printf_proc_t printf_proc,
              datastore_config_type_t ftype,
              char *file_name,
              bool is_with_stats,
              cmd_dump_proc_t dump_proc);

#endif /* __CMD_DUMP_H__ */
