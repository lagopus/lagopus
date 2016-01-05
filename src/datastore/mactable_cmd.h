/* %COPYRIGHT% */

/**
 * @file	mactable_cmd.h
 */

#ifndef __MACTABLE_CMD_H__
#define __MACTABLE_CMD_H__

#include "cmd_common.h"

/**
 * Initialize mactable cmd.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
mactable_cmd_initialize(void);

/**
 * Finalize mactable cmd.
 *
 *     @retval	void
 */
void
mactable_cmd_finalize(void);


#endif /* __MACTABLE_CMD_H__ */
