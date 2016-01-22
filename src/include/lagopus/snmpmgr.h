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

/**
 *	@file snmpmgr.h
 */

/**
 * Initialize the SNMP subagent module.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_POSIX_API_ERROR		Something wrong
 *	with Posix API calls.
 *	@retval	LAGOPUS_RESULT_SNMP_API_ERROR		Something wrong in
 *	the module initialization as a Net-SNMP agent.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS		'thdptr' is NULL
 *	(return only if is_thread is not false).
 */
lagopus_result_t
snmpmgr_initialize(void *is_thread, lagopus_thread_t **thdptr);

/**
 * Start the SNMP subagent module.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_NOT_STARTED		The module is not yet initialized.
 *	@retval	LAGOPUS_RESULT_ALREADY_EXISTS		The module already started.
 *  @return Otherwize, identical to the lagopus_thread_start()
 *	if the module is initialized as a thread.
 *
 *	@details Start the SNMP subagent module to open a session to the AgentX master agent,
 *  and also start a thread if the module is initialized as a thread.
 */
lagopus_result_t
snmpmgr_start(void);

/**
 * Shutdown the SNMP subagent module.
 *
 *	@param[in] level		The shutdown graceful level, one of;
 *	SHUTDOWN_RIGHT_NOW: shutdown the SNMP subagent module RIGHT NOW;
 *	SHUTDOWN_GRACEFULLY: shutdown the SNMP subagent module as graceful as it can
 *	be.
 *
 *	@retval	LAGOPUS_RESULT_OK	Succeeded.
 *  @return Otherwize, identical to the lagopus_thread_start()
 *	if the module is initialized as a thread
 */
lagopus_result_t
snmpmgr_shutdown(shutdown_grace_level_t level);

/**
 * Stop the SNMP subagent module forcibly.
 *
 *	@retval	LAGOPUS_RESULT_OK	Succeeded.
 *	@return Otherwize, identical to
 *	the lagopus_thread_is_cancel() or the lagopus_thread_cancel().
 */
lagopus_result_t
snmpmgr_stop(void);

/**
 * Finalize the SNMP subagent module.
 */
void
snmpmgr_finalize(void);

/**
 * Poll a SNMP request from AgentX master agent.
 *
 *	@param nsec		A polling time in the nanoseconds.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR		Something wrong with the Posix API calls.
 *	@retval LAGOPUS_RESULT_TIMEDOUT		There is no request in `nsec` nano seconds.
 *	@retval LAGOPUS_RESULT_SNMP_API_ERROR		Something wrong with the Net-SNMP API calls.
 */
lagopus_result_t
snmpmgr_poll(lagopus_chrono_t nsec);
