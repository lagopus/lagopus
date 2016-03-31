/* %COPYRIGHT% */

/**
 * @file	route_cmd.h
 */

#ifndef __ROUTE_CMD_H__
#define __ROUTE_CMD_H__

#include "cmd_common.h"

/**
 * Initialize route cmd.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
route_cmd_initialize(void);

/**
 * Finalize route cmd.
 *
 *     @retval	void
 */
void
route_cmd_finalize(void);

#endif /* __ROUTE_CMD_H__ */
