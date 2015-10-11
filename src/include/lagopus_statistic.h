#ifndef __LAGOPUS_STATISTIC_H__
#define __LAGOPUS_STATISTIC_H__





/**
 *	@file	lagopus_statistic.h
 */





typedef struct lagopus_statistic_struct	*lagopus_statistic_t;





/**
 * Create a statistic.
 *
 *	@param[in,out]	sptr	A pointer to a statistic.
 *	@param[in]	name	Name of the statistic.
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_create(lagopus_statistic_t *sptr, const char *name);


/**
 * Find a statistic by name.
 *
 *	@param[out]	sptr	A pointer to a statistic.
 *	@param[in]	name	Name of the statistic.
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND	Failed, not found.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_find(lagopus_statistic_t *sptr, const char *name);


/**
 * Destroy a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 */
void
lagopus_statistic_destroy(lagopus_statistic_t *sptr);


/**
 * Destroy a statistic by name.
 *
 *	@param[in]	name	Name of the statistic.
 */
void
lagopus_statistic_destroy_by_name(const char *name);


/**
 * Record a value to a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	val	A value.
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_record(lagopus_statistic_t *sptr, int64_t val);


/**
 * Reset a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_reset(lagopus_statistic_t *sptr);


/**
 * Acquire # of the sample from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *
 *	@retval	>=0				# of the sample.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_sample_n(lagopus_statistic_t *sptr);


/**
 * Acquire the minimum value from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_min(lagopus_statistic_t *sptr, int64_t *valptr);


/**
 * Acquire the maximum value from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_max(lagopus_statistic_t *sptr, int64_t *valptr);


/**
 * Acquire the average of a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_statistic_average(lagopus_statistic_t *sptr, double *valptr);


/**
 * Acquire the standard deviation of a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *	@param[in]	is_ssd	\b true) returns the Sample Standard Deviation (a.k.a. Unbiased Standard Deviation) instead of the standard deviation;
 *
 *	@retval	LAGOPUS_RESULT_OK		Suceeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.

 */
lagopus_result_t
lagopus_statistic_sd(lagopus_statistic_t *sptr, double *valptr, bool is_ssd);





#endif /* ! __LAGOPUS_STATISTIC_H__ */
