/* %COPYRIGHT% */

#ifndef __LAGOPUS_CALLOUT_TASK_STATE_H__
#define __LAGOPUS_CALLOUT_TASK_STATE_H__





/**
 *	@file	lagopus_callout_task_state.h
 */





/**
 * The callout task status.
 */
typedef enum {
  TASK_STATE_UNKNOWN = 0,
  TASK_STATE_CREATED,	/** Created. */
  TASK_STATE_ENQUEUED,	/** Submitted/enqueued. */
  TASK_STATE_DEQUEUED,	/** Decqueud for execution. */
  TASK_STATE_EXECUTING,	/** Just executing at the moment. */
  TASK_STATE_EXECUTED,	/** Executed. */
  TASK_STATE_EXEC_FAILED,	/** Execution failed. */
  TASK_STATE_CANCELLED,	/** Cancelled. */
  TASK_STATE_DELETING	/** Just deleting at the moment. */
} callout_task_state_t;


typedef callout_task_state_t	lagopus_callout_task_state_t;





#endif /* __LAGOPUS_CALLOUT_TASK_STATE_H__ */
