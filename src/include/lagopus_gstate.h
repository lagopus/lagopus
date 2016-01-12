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

#ifndef __LAGOPUS_GSTATE_H__
#define __LAGOPUS_GSTATE_H__





/**
 * @file	lagopus_gstate.h
 */





typedef enum {
  GLOBAL_STATE_UNKNOWN = 0,
  GLOBAL_STATE_INITIALIZING,
  GLOBAL_STATE_INITIALIZED,
  GLOBAL_STATE_STARTING,
  GLOBAL_STATE_STARTED,
  GLOBAL_STATE_REQUEST_SHUTDOWN,
  GLOBAL_STATE_ACCEPT_SHUTDOWN,
  GLOBAL_STATE_SHUTTINGDOWN,
  GLOBAL_STATE_SHUTDOWN,
  GLOBAL_STATE_FINALIZING,
  GLOBAL_STATE_FINALIZED
} global_state_t;
#define IS_VALID_GLOBAL_STATE(s)                                \
  ((((int)(s) > (int)GLOBAL_STATE_UNKNOWN) &&                   \
    ((int)(s) <= (int)GLOBAL_STATE_FINALIZED)) ? true : false)
#define IS_GLOBAL_STATE_SHUTDOWN(s)                                     \
  (((int)(s) >= (int)GLOBAL_STATE_ACCEPT_SHUTDOWN) ? true : false)
#define IS_GLOBAL_STATE_KINDA_SHUTDOWN(s)                               \
  (((int)(s) >= (int)GLOBAL_STATE_REQUEST_SHUTDOWN) ? true : false)


typedef enum {
  SHUTDOWN_UNKNOWN = 0,
  SHUTDOWN_RIGHT_NOW,
  SHUTDOWN_GRACEFULLY
} shutdown_grace_level_t;
#define IS_VALID_SHUTDOWN(s) \
  (((int)(s) > (int)SHUTDOWN_UNKNOWN) &&                \
   ((int)(s) <= (int)SHUTDOWN_GRACEFULLY)) ? true : false





/**
 * Set the global state.
 *
 *	@param[in]	s	A global state.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION
 *	Failed, invalid global state.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t	global_state_set(global_state_t s);


/**
 * Get the global state.
 *
 *	@param[out]	sptr	A pointer to a global state.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t	global_state_get(global_state_t *sptr);


/**
 * Wait for change of the global state.
 *
 *	@param[in]	s_wait_for	A desired global state.
 *	@param[out]	cur_sptr	A pointer to a global state after
 *	the change.
 *	@param[out]	cur_gptr	A pointer to a shutdown grace level
 *	after the change.
 *	@param[in]	nsec		A timeout (in nsec).
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the current global state is more further than the
 *	\b s_wait_for, it always returns \b LAGOPUS_RESULT_OK.
 *	@details If the global state is advanced while waiting more
 *	further than the \b s_wait_for, it also returns
 *	LAGOPUS_RESULT_OK, EXCLUDE the case if the current global
 *	state changes to shutdown state. In this case, it returns
 *	the \b LAGOPUS_RESULT_NOT_OPERATIONAL and the caller must
 *	check the \b cur_gptr to prepare/invoke for appropriate
 *	shutdown sequence.
 */
lagopus_result_t
global_state_wait_for(global_state_t s_wait_for,
                      global_state_t *cur_sptr,
                      shutdown_grace_level_t *cur_gptr,
                      lagopus_chrono_t nsec);

/**
 * Wait for change of the global state to shutdown state.
 *
 *	@param[out]	cur_gptr	A pointer to a shutdown grace level
 *	after the change.
 *	@param[in]	nsec		A timeout (in nsec).
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This is equivalent to calling;
 *	global_state_wait_for(\b GLOBAL_STATE_REQUEST_SHUTDOWN,
 *	\b NULL, \b cur_gptr, \b nsec).
 *
 *	@details After calling this procedure and it returns \b
 *	LAGOPUS_RESULT_OK, (one of) the caller must call
 *	global_state_set(\b GLOBAL_STATE_ACCEPT_SHUTDOWN) or caller of
 *	the global_state_request_shutdown() blocks forever.
 */
lagopus_result_t
global_state_wait_for_shutdown_request(shutdown_grace_level_t *cur_gptr,
                                       lagopus_chrono_t nsec);


/**
 * Request shudtwon.
 *
 *	@param[in]	l	A shutdown grace level.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION
 *	Failed, invalid global state.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details Calling this procedure blocks the caller until the
 *	global state shanges to the \b GLOBAL_STATE_ACCEPT_SHUTDOWN.
 */
lagopus_result_t
global_state_request_shutdown(shutdown_grace_level_t l);


/**
 * Clean an internal state the global state tracker up after the
 * cancellation of the caller threads.
 */
void	global_state_cancel_janitor(void);


/**
 * Not documented on purposely.
 *
 *	@details Use this only in the unit tests.
 */
void	global_state_reset(void);





#endif /* __LAGOPUS_GSTATE_H__ */
