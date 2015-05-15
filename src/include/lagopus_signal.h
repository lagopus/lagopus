/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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


#ifndef __LAGOPUS_SIGNAL_H__
#define __LAGOPUS_SIGNAL_H__





/**
 *      @file lagopus_signal.h
 */





/**
 * A MT-Safe signal(2).
 */
lagopus_result_t
lagopus_signal(int signum, sighandler_t new, sighandler_t *oldptr);


/**
 * Fallback the signal handling mechanism to the good-old-school
 * semantics.
 */
void    lagopus_signal_old_school_semantics(void);





#endif /* ! __LAGOPUS_SIGNAL_H__ */
