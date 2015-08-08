#ifndef __LAGOPUS_CHRONO_H__
#define __LAGOPUS_CHRONO_H__





/**
 *	@file	lagopus_chrono.h
 */





lagopus_chrono_t
lagopus_chrono_now(void);


lagopus_result_t
lagopus_chrono_to_timespec(struct timespec *dstptr,
                           lagopus_chrono_t nsec);


lagopus_result_t
lagopus_chrono_to_timeval(struct timeval *dstptr,
                          lagopus_chrono_t nsec);


lagopus_result_t
lagopus_chrono_from_timespec(lagopus_chrono_t *dstptr,
                             const struct timespec *specptr);


lagopus_result_t
lagopus_chrono_from_timeval(lagopus_chrono_t *dstptr,
                            const struct timeval *valptr);


lagopus_result_t
lagopus_chrono_nanosleep(lagopus_chrono_t nsec,
                         lagopus_chrono_t *remptr);





#endif /* ! __LAGOPUS_CHRONO_H__ */
