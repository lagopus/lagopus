#ifndef __LAGOPUS_NUMA_H__
#define __LAGOPUS_NUMA_H__





/**
 *	@file	lagopus_numa.h
 */





/**
 * Allocate dynamic memory on the nearest NUMA node which the
 * specified CPU belongs.
 *
 *	@param[in]	sz	A size to allocate (in bytes.)
 *	@param[in]	cpu	A cpu/core, identical to an index for
 *				the \b cpu_set_t. Passing a negative
 *				value makes the API returns \b
 *				malloc(\b sz )
 *
 *	@retval	!=NULL		Succeeded, the allocated memory.
 *	@retval NULL		Failed.
 */
void *	lagopus_malloc_on_cpu(size_t sz, int cpu);


/**
 * Free memory allocated by the \b lagopus_malloc_on_cpu().
 *
 *	@param[in]	p	A pointer to memory.
 */
void	lagopus_free_on_cpu(void *p);


/**
 * Returns the NUMA is enabled or not.
 *
 *	@retval	true	The NUMA is enabled.
 *	@retval	true	The NUMA is disabled.
 */
bool	lagopus_is_numa_enabled(void);





#endif /* ! __LAGOPUS_NUMA_H__ */
