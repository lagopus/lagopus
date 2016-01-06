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

#ifndef __LAGOPUS_MODULE_APIS_H__
#define __LAGOPUS_MODULE_APIS_H__





/**
 *	@file lagopus_module_apis.h
 */





#define LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE	1000


/**
 * Initialize the module.
 *
 *	@param[in]	argc	A # of the argments which are provided for the main().
 *	@param[in]	argv	The arguments which are provided for the main().
 *	@param[in]	extarg	An extra argument (if needed).
 *	@param[out]	thdptr	A pointer to a thread (if used).
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval Any appropriate result code defined in
 *	lagopus_error.h. If tehre are no appropriate one, the
 *	module implementer could define new one.
 *
 *	@details Any modules can abort by calling lagopus_exit_fatal()
 *	if needed/desired.
 *
 *	@details If the module don't use any threads, the returned
 *	thdptr could be NULL.
 *
 *	@details The caller doesn't care how many time and when it
 *	calls this initializer so the callee must take care of those
 *	issues by itself. In order to do this, the module implementer
 *	would be required to use pthread_once mechanism.
 *
 *	@details the \b argc and the \b argv are provided for the
 *	module specific command line arguments parsing, if needed. And
 *	note that the \b argv is THE MOST STRICTLY READ-ONLY. DON'T
 *	OVERWRITE FOR OTHER MODULES' PROPER OPERATION.
 */
typedef lagopus_result_t
(*lagopus_module_initialize_proc_t)(int argc,
                                    const char *const argv[],
                                    void *extarg,
                                    lagopus_thread_t **thdptr);


/**
 * Start the module.
 *
 *	@return Identical to the lagopus_thread_start().
 *
 *	@details If the module don't use any threads, an appropriate
 *	return value is expected.
 */
typedef lagopus_result_t
(*lagopus_module_start_proc_t)(void);


/**
 * Shutdown the module.
 *
 *	@param[in] level The shutdown graceful level, one of;
 *	SHUTDOWN_RIGHT_NOW: shutdown the module RIGHT NOW;
 *	SHUTDOWN_GRACEFULLY: shutdown the module as graceful as it can
 *	be.
 *
 *	@retval	LAGOPUS_RESULT_OK	Succeeded.
 *	@retval Any appropriate result code defined in
 *	lagopus_error.h. If tehre are no appropriate one, the
 *	module implementer could define new one.
 *
 *	@details If the module don't use any threads, an appropriate
 *	return value is expected.
 *
 *	@details The caller expects that the callee calls any module
 *	sepcific termination methods suitable for the \b level
 *	argument and just return WITHOUT ANY synchronization between
 *	corresponding threads. Then the caller wait the termination of
 *	the threads with appropriate timeout value by calling the
 *	lagopus_thread_wait(). If the wait failed, then the caller
 *	calls the modx_stop() to stop the threads forcibly.
 */
typedef lagopus_result_t
(*lagopus_module_shutdown_proc_t)(shutdown_grace_level_t level);


/**
 * Stop the module forcibly.
 *
 *	@return Identical to the lagopus_thread_cancel().
 *
 *	@details If the module don't use any threads, an appropriate
 *	return value is expected. Otherwise the caller expects that
 *	the callee just call the \b lagopus_thread_cancel() and return
 *	the return value.
 *
 *	@details Futhermore, the caller doesn't care how many time it
 *	calls the stop function. If the module implementer needs to
 *	avoid the multiple call of the function, the module
 *	implementer must provide the mechanisms to do that. Note that
 *	the module implementer doesn't have to do that if one use the
 *	lagopus thread APIs and use the \b lagopus_thread_cancel().
 */
typedef lagopus_result_t
(*lagopus_module_stop_proc_t)(void);


/**
 * Finalize the module.
 *
 *	@details The callee is expected to destroy/freeup all the
 *	resources the module acquired.
 *
 *	@details Lile the initializer, the caller doesn't care how
 *	many time when it calls this initializer (also any sequential
 *	dependencies aren't cared) so the callee must take care of
 *	those issues by itself. In order to do this, the module
 *	implementer would be required to use pthread_once mechanism.
 */
typedef void
(*lagopus_module_finalize_proc_t)(void);


/**
 * Emit the module usage.
 *
 *	@param[in]	fd	A file descriptor emit to.
 *
 *	@details If the initialize function parses the given command
 *	line arguments, the module implementer should ptovide this
 *	function to output the command line option usage.
 */
typedef void
(*lagopus_module_usage_proc_t)(FILE *fd);





/**
 * Register a module.
 *
 *	@param[in]	name	A name of the module.
 *	@param[in]	init_proc	An initialize function.
 *	@param[in]	extarg		An extra argument for the initialize function (\b NULL allowed).
 *	@param[in]	start_proc	A start function.
 *	@param[in]	shutdown_proc	A shutdown function.
 *	@param[in]	stop_proc	A stop function (\b NULL allowed.)
 *	@param[in]	finalize_proc	A finalize function.
 *	@param[in]	usage_proc	A usage function (\b NULL allowed.)
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_module_register(const char *name,
                        lagopus_module_initialize_proc_t init_proc,
                        void *extarg,
                        lagopus_module_start_proc_t start_proc,
                        lagopus_module_shutdown_proc_t shutdown_proc,
                        lagopus_module_stop_proc_t stop_proc,
                        lagopus_module_finalize_proc_t finalize_proc,
                        lagopus_module_usage_proc_t usage_proc);


/**
 * Emit all modules' usage.
 *
 *	@param[in]	fd	A file descriptor emit to.
 */
void
lagopus_module_usage_all(FILE *fd);


/**
 * Initialize all the modules.
 *
 *	@param[in]	argc	A # of the argments which are provided for the main().
 *	@param[in]	argv	The arguments which are provided for the main()
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval <0 Any other faiulre(s).
 */
lagopus_result_t
lagopus_module_initialize_all(int argc, const char *const argv[]);


/**
 * Start all the modules.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval <0 Any other faiulre(s).
 */
lagopus_result_t
lagopus_module_start_all(void);


/**
 * Shutdown all the modules.
 *
 *	@param[in]	level	A shutdown graceful level.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval <0 Any other faiulre(s).
 */
lagopus_result_t
lagopus_module_shutdown_all(shutdown_grace_level_t level);


/**
 * Stop all the modules, forcibly.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval <0 Any other faiulre(s).
 */
lagopus_result_t
lagopus_module_stop_all(void);


/**
 * Wait all the modules.
 *
 *	@param[in]	nsec	Wait timeout (in nano second).

 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval <0 Any other faiulre(s).
 */
lagopus_result_t
lagopus_module_wait_all(lagopus_chrono_t nsec);


/**
 * Finalize all the modules.
 */
void
lagopus_module_finalize_all(void);


/**
 * Find a module by name.
 *
 *	@param[in]	name	A name of the module to find.
 *
 *	@retval >=0				Found. (an index of the module.)
 *	@retval LAGOPUS_RESULT_NOT_FOUND	Failed, not found.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_module_find(const char *name);





/*
 * Undocumeted on purpose.
 */
bool	lagopus_module_is_finalized_cleanly(void);
bool	lagopus_module_is_unloading(void);





#endif /* __LAGOPUS_MODULE_APIS_H__ */
