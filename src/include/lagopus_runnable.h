#ifndef __LAGOPUS_RUNNABLE_H__
#define __LAGOPUS_RUNNABLE_H__





/**
 *	@file	lagopus_runnable.h
 */





#include "lagopus_runnable_funcs.h"





#ifndef RUNNABLE_T_DECLARED
typedef struct lagopus_runnable_record 	*lagopus_runnable_t;
#define RUNNABLE_T_DECLARED
#endif /* ! RUNNABLE_T_DECLARED */





/**
 * Create a runnable.
 *
 *	@param[out]	rptr	A pointer to a created runnable.
 *	@param[in]	sz	A memory allocation size for this object
 *	(in bytes.)
 *	@param[in]	func	A runnable procedure.
 *	@param[in]	arg	An argument for the\b func (\b NULL allowed.)
 *	@param[in]	freeup_proc	A freeup prosedure (\b NULL allowed.)
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 * @details If the \b rptr is \b NULL, it allocates a memory area and
 * the allocated area is always free'd by calling \b
 * lagopus_runner_destroy(). In this case if the \b sz is greater than
 * the original object size, \b sz bytes momory area is
 * allocated. Otherwise if the \b rptr is not NULL the pointer given
 * is used as is.
 */
lagopus_result_t
lagopus_runnable_create(lagopus_runnable_t *rptr,
                        size_t sz,
                        lagopus_runnable_proc_t func,
                        void *arg,
                        lagopus_runnable_freeup_proc_t freeup_proc);


/**
 * Destroy a runnable.
 *
 *	@param[in]	rptr	A pointer to a runnable.
 *
 * @details If the \b freeup_proc was specified at creation, the \b
 * freeup_proc is called BEFORE free the \b *rptr up.
 */
void
lagopus_runnable_destroy(lagopus_runnable_t *rptr);


/**
 * Execute a runnable.
 *
 *	@param[in]	rptr	A pointer to a runnable.
 */
lagopus_result_t
lagopus_runnable_start(const lagopus_runnable_t *rptr);





#endif /* ! __LAGOPUS_RUNNABLE_H__ */
