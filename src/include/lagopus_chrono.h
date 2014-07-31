/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
