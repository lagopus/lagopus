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


#ifndef __LAGOPUS_RUNNABLE_FUNCS_H__
#define __LAGOPUS_RUNNABLE_FUNCS_H__




/**
 *      @file   lagopus_runnable_funcs.h
 */





#ifndef RUNNABLE_T_DECLARED
typedef struct lagopus_runnable_record  *lagopus_runnable_t;
#define RUNNABLE_T_DECLARED
#endif /* ! RUNNABLE_T_DECLARED */





/**
 * A procedure for a runnable.
 *
 *      @param[in]      rptr    A runnable.
 *      @param[in]      arg     An argument.
 *
 * @details If any resoures acquired in the function must be released
 * before returning. Doing this must be the functions's responsibility.
 *
 * @details This function must not loop infinitely, but must return in
 * appropriate amount of execution time.
 */
typedef void (*lagopus_runnable_proc_t)(const lagopus_runnable_t *rptr,
                                        void *arg);


/**
 * Free a runnable up.
 *
 *      @param[in]      rptr    A pointer to a runnable.
 *
 * @details Don't free the \b *rptr itself up.
 */
typedef void (*lagopus_runnable_freeup_proc_t)(lagopus_runnable_t *rptr);





#endif /* ! __LAGOPUS_RUNNABLE_FUNCS_H__ */
