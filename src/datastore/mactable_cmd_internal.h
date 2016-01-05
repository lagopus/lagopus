/* %COPYRIGHT% */

/**
 * @file	mactable_cmd_internal.h
 */

#ifndef __MACTABLE_CMD_INTERNAL_H__
#define __MACTABLE_CMD_INTERNAL_H__

#include "cmd_common.h"

/**
 * Parse command for mactable dump.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @param[in]	argc	A # of tokens in the \b argv.
 *     @param[in]	argv	An array of tokens.
 *     @param[in]	hptr	A hashmap which has related objects.
 *     @param[in]	proc	An update proc.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
STATIC lagopus_result_t
mactable_cmd_parse(datastore_interp_t *iptr,
               datastore_interp_state_t state,
               size_t argc, const char *const argv[],
               lagopus_hashmap_t *hptr,
               datastore_update_proc_t u_proc,
               datastore_enable_proc_t e_proc,
               datastore_serialize_proc_t s_proc,
               datastore_destroy_proc_t d_proc,
               lagopus_dstring_t *result);


#endif /* __MACTABLE_CMD_INTERNAL_H__ */
