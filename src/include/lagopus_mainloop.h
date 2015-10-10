/* %COPYRIGHT% */

#ifndef __LAGOPUS_MAINLOOP_H__
#define __LAGOPUS_MAINLOOP_H__





/**
 *	@file	lagopus_mainloop.h
 */





/**
 * The signature of the main loop function.
 *
 *	@paran[in] argc The \b argc for the \b
 *			lagopus_module_initialize_all().
 *	@paran[in] argv The \b argv for the \b
 *			lagopus_module_initialize_all().
 *
 *	@retval LAGOPUS_RESULT_OK	Succeeded.
 *	@retval <0			Any failures.
 *
 *	@details This procedure must block until the application is
 *	required to shutdown by any other threads.
 *
 *	@details The most likely this function should firstly calls
 *	the \b lagopus_mainloop_prologue(), then waits for the
 *	shutdown request (mainly by callling the \b
 *	global_state_wait_for_shutdown_request()), finally calls the
 *	\b lagopus_mainloop_epilogue().
 */
typedef lagopus_result_t (*lagopus_mainloop_proc_t)(
    int argc, const char * const argv[]);


/**
 * The signature of the module startup hook functions.
 *
 *	@retval LAGOPUS_RESULT_OK	Succeeded.
 *	@retval <0			Any failures.
 *
 *	@detals Use this function to install a hook for module
 *	startup, for the lagopus_mainloop_prologue() to provide a
 *	custom main loop function.
 */
typedef lagopus_result_t (*lagopus_mainloop_module_startup_hook_proc_t)(void);





/**
 * Enter the application main loop.
 *
 *	@paran[in] argc The \b argc for the \b
 *			lagopus_module_initialize_all().
 *	@paran[in] argv The \b argv for the \b
 *			lagopus_module_initialize_all().
 *	@param[in]	proc	The main loop function (NULL allowed.)
 *
 *	@retval	LAGOPUS_RESULT_OK	Succeeded.
 *	@retval <0			Any failures.
 *
 *	@details If the \b proc is NULL, the default main loop
 *	function, that a) initializa and start all the registered
 *	module; b) Wait for the shutdown request; c) If got the
 *	request, shutdown and finalize all the modules; d) Exit the
 *	application.
 */
lagopus_result_t
lagopus_mainloop(int argc, const char * const argv[],
                 lagopus_mainloop_proc_t proc);


/**
 * The prologue of the application main loop.
 *
 *	@paran[in] argc The \b argc for the \b
 *			lagopus_module_initialize_all().
 *	@paran[in] argv The \b argv for the \b
 *			lagopus_module_initialize_all().
 *	@param[in] hook A module startup hook (NULL allowed.)
 *
 *	@retval	LAGOPUS_RESULT_OK	Succeeded.
 *	@retval <0			Any failures.
 *
 *	@details This function mainly performs the initialization and
 *	startup of the registerd modules. If the \b hook is not \b
 *	NULL, it is called between the initialization and the startup
 *	of the modules. Use this function to provide a custom main
 *	loop function.
 */
lagopus_result_t
lagopus_mainloop_prologue(int argc, const char * const argv[], 
                          lagopus_mainloop_module_startup_hook_proc_t hook);


/**
 * The epilogue of the application main loop.
 *
 *	@param[in]	l	The shutdown grace level.
 *	@param[in]	to	The shutdown timeout.
 *
 *	@retval	LAGOPUS_RESULT_OK	Succeeded.
 *	@retval <0			Any failures.
 *
 *	@detail This function mainly performs the shutdown and
 *	finalization of the registered modules. Use this function to
 *	provide a custom main loop function.
 */
lagopus_result_t
lagopus_mainloop_epilogue(shutdown_grace_level_t l, lagopus_chrono_t to);





#endif /* __LAGOPUS_MAINLOOP_H__ */
