/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
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
 *	@file lagopus_signal.h
 */





#ifdef __FreeBSD__
#define sighandler_t sig_t
#endif /* __FreeBSD__ */
#ifdef __NetBSD__
typedef void (*sighandler_t)(int);
#endif /* __NetBSD__ */


#define SIG_CUR	__s_I_g_C_u_R__





void	__s_I_g_C_u_R__(int sig);





/**
 * A MT-Safe signal(2).
 *
 *	@param[in]	signum	A signal.
 *	@param[in]	new	A new signal handler.
 *	@param[out]	oldptr	A pointer to the current/old handler.
 *
 *	@retval	LAGOPUS_RSULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, POSIX error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details if the \b new is \b SIG_CUR, the API just returns the
 *	old signal handler to \b *oldptr.
 */
lagopus_result_t
lagopus_signal(int signum, sighandler_t new, sighandler_t *oldptr);


/**
 * Fallback the signal handling mechanism to the good-old-school
 * semantics.
 */
void	lagopus_signal_old_school_semantics(void);





#endif /* ! __LAGOPUS_SIGNAL_H__ */
