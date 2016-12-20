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

#ifndef __DATASTORE_APIS_H__
#define __DATASTORE_APIS_H__





/**
 *	@file	datastore_apis.h
 */





#include "datastore_interp_state.h"
#include "lagopus_session.h"





typedef struct datastore_interp_record 	*datastore_interp_t;


typedef char 	*(*datastore_gets_proc_t)(char *s, int size, void *stream);
typedef int	(*datastore_printf_proc_t)(void *stream, const char *fmt, ...);


typedef enum {
  DATASTORE_CONFIG_TYPE_UNKNOWN = 0,
  DATASTORE_CONFIG_TYPE_STREAM_FD,
  DATASTORE_CONFIG_TYPE_STREAM_SESSION,
  DATASTORE_CONFIG_TYPE_FILE
} datastore_config_type_t;





/**
 * The signature of object update functions.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	state	A status of the interpreter.
 *	@param[in]	obj	An object.
 *	@param[out]	result	A result/output string (NULL allowed).
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval <0	TBD or command specific error(s).
 */
typedef lagopus_result_t
(*datastore_update_proc_t)(datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           void *obj,
                           lagopus_dstring_t *result);


/**
 * The signature of object enable/disable/query functions.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	state	A status of the interpreter.
 *	@param[in]	obj	An object.
 *	@param[in,out]	currentp A status of the current object returned, \b true: enable, \b false: disable.
 *	@param[in]	enabled A status of the object: \b true: enable, \b false: disable. Only meaningful when the \b statusp is NULL.
 *	@param[out]	result	A result/output string (NULL allowed).
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval <0	TBD or command specific error(s).
 */
typedef lagopus_result_t
(*datastore_enable_proc_t)(datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           void *obj,
                           bool *currentp,
                           bool enabled,
                           lagopus_dstring_t *result);


/**
 * The signature of object serialize functions.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	state	A status of the interpreter.
 *	@param[in]	obj	An object.
 *	@param[out]	result	A result/output string (NULL allowed).
 *
 *	@retval	LAGOPUS_RESULT_OK		oSucceeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval <0	TBD or command specific error(s).
 *
 *	@details The \b result must be ended with "\n".
 */
typedef lagopus_result_t
(*datastore_serialize_proc_t)(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              const void *obj,
                              lagopus_dstring_t *result);


/**
 * The signature of object destroy functions.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	state	A status of the interpreter.
 *	@param[in]	obj	An object.
 *	@param[out]	result	A result/output string (NULL allowed).
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval <0	TBD or command specific error(s).
 */
typedef lagopus_result_t
(*datastore_destroy_proc_t)(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            void *obj,
                            lagopus_dstring_t *result);


/**
 * The signature of object compare function for object ordering.
 *
 *	@param[in]	obj1, obj2	objects to be comapred
 *	@param[out]	result	A result (>0: obj1 > obj2, 0: obj1 == obj2, <0: obj1 < obj2)
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval <0	TBD or command specific error(s).
 */
typedef lagopus_result_t
(*datastore_compare_proc_t)(const void *obj1, const void *obj2, int *result);


/**
 * The signature of object name acquisition function.
 *
 *	@param[in]	obj	An object
 *	@param[out]	namep	A name of the \b obj returned.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval <0	TBD or command specific error(s).
 *
 *	@details The returned \b *namep must not be modified.
 */
typedef lagopus_result_t
(*datastore_getname_proc_t)(const void *obj, const char **namep);


/**
 * The signature of object duplicate function.
 *
 *	@param[in]	obj	An object
 *	@param[in]	namespace	A namespace of the duplication obj.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval <0	TBD or command specific error(s).
 *
 *	@details The returned \b *namep must not be modified.
 */
typedef lagopus_result_t
(*datastore_duplicate_proc_t)(const void *obj, const char* namespace);


/**
 * The signature of each command interpreter.
 *
 *	@param[in]	iptr	An interpreter.
 *	@paran[in]	state	A status of the interpreter.
 *	@param[in]	argc	A # of tokens in the \b argv.
 *	@param[in]	argv	An array of tokens.
 *	@param[in]	hptr	A hashmap which has related objects.
 *	@param[in]	proc	An update proc.
 *	@param[out]	result	A result/output string (NULL allowed).
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval <0	TBD or command specific error(s).
 *
 * @details If the \b stete is \b DATASTORE_INTERP_MODE_AUTO_COMMIT,
 * the \b proc must be called for update the object.
 */
typedef lagopus_result_t
(*datastore_command_proc_t)(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            size_t argc, const char *const argv[],
                            lagopus_hashmap_t *hptr,
                            datastore_update_proc_t u_proc,
                            datastore_enable_proc_t e_proc,
                            datastore_serialize_proc_t s_proc,
                            datastore_destroy_proc_t d_proc,
                            lagopus_dstring_t *result);





/**
 * Register an object database.
 *
 *	@param[in]	argv0	A table name. Supposed to be same as
 *				the first argument of for the
 *				\b datastore_interp_register_command().
 *	@param[in]	tptr	A hash map.
 *	@param[in]	u_proc	An update function.
 *	@param[in]	e_proc	An enable function.
 *	@param[in]	s_proc	A serialize function.
 *	@param[in]	d_proc	A destroy function.
 *	@param[in]	c_proc	A compare function (NULL allowed.)
 *	@param[in]	n_proc	A getname function.
 *	@param[in]	dup_proc	A duplicate function.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval <0	TBD.
 *
 * @details In order to provide the atomicity of the successive
 * operation, all the side effects must be applied in serial. To do
 * so, at the commitments, the \b u_proc (and maybe \b e_proc if
 * needed) is called for each object in the \b tptr.
 *
 * @details The \b s_proc is called when whole the datastore is
 * serialized/saved for each object in the \b tptr.
 */
lagopus_result_t
datastore_register_table(const char *argv0,
                         lagopus_hashmap_t *tptr,
                         datastore_update_proc_t u_proc,
                         datastore_enable_proc_t e_proc,
                         datastore_serialize_proc_t s_proc,
                         datastore_destroy_proc_t d_proc,
                         datastore_compare_proc_t c_proc,
                         datastore_getname_proc_t n_proc,
                         datastore_duplicate_proc_t dup_proc);


/**
 * Find an object database.
 *
 *	@param[in]	argv0	A table name. Supposed to be same as
 *				the first argument of for the
 *				\b datastore_interp_register_command().
 *	@param[out]	tptr	A hash map returned (NULL allowed.)
 *	@param[out]	u_pptr	A update function returned (NULL allowed.)
 *	@param[out]	e_pptr	An enable function returned (NULL allowed.)
 *	@param[out]	s_pptr	A serialize function returned (NULL allowed.)
 *	@param[out]	d_pptr	A destroy function returned (NULL allowed.)
 *	@param[out]	c_pptr	A compare function returned (NULL allowed.)
 *	@param[out]	n_pptr	A getname function returned (NULL allowed.)
 *	@param[out]	dup_pptr	A duplicate function returned (NULL allowed.)
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NOT_FOUND	Failed, not found.
 *	@retval <0	TBD.
 */
lagopus_result_t
datastore_find_table(const char *argv0,
                     lagopus_hashmap_t *tptr,
                     datastore_update_proc_t *u_pptr,
                     datastore_enable_proc_t *e_pptr,
                     datastore_serialize_proc_t *s_pptr,
                     datastore_destroy_proc_t *d_pptr,
                     datastore_compare_proc_t *c_pptr,
                     datastore_getname_proc_t *n_pptr,
                     datastore_duplicate_proc_t *dup_pptr);


/**
 * Get all objects in a database.
 *
 *	@param[in]	argv0	A table name. Supposed to be same as
 *				the first argument of for the
 *				\b datastore_register_table().
 *	@param[out]	listp	A list of the objjects returned.
 *	@param[in]	do_sort	If \b true, the \b listp is soted which
 *				uses a compare function provided with the
 *				\b database_register_table().
 *
 *	@retval	>= 0				Succeeded, # of objects returned.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_NOT_FOUND	Failed, database specified by \b argv0 not found.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details A caller must \b free() the *listp after it is not needed anymore.
 */
lagopus_result_t
datastore_get_objects(const char *argv0, void ***listp, bool do_sort);





/**
 * Create an interpreter.
 *
 *	@param[out]	iptr	A pointer to an interpreter to be created.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_create_interp(datastore_interp_t *iptr);


/**
 * Shutdown an interpreter.
 *
 *	@param[in]	iptr	An interpreter.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_shutdown_interp(datastore_interp_t *iptr);


/**
 * Destory an interpreter.
 *
 *	@param[in]	iptr	An interpreter.
 */
void
datastore_destroy_interp(datastore_interp_t *iptr);


/**
 * Acquire a current state of an interpreter.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[out]	sptr	A pointer of the state returned.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_get_interp_state(const datastore_interp_t *iptr,
                           datastore_interp_state_t *sptr);





/**
 * Regsiter a configurator.
 *
 *	@param[in]	name	A name of the configurater.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 * @details In order to introduce a concept of the monitor
 * (lock/unlock of the configururation database), each configurater
 * must has an unique name. Use this API to register the name,
 * whenever any configurater become avairable.
 */
lagopus_result_t
datastore_register_configurator(const char *name);


/**
 * Unregister a configurator.
 *
 *	@param[in]	name	A name of the configurater.
 *
 * @details Use this API whenever any configurater become unavairable.
 */
void
datastore_unregister_configurator(const char *name);





/**
 * Register a command into an interpretor.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	argv0	A table name. Supposed to be same as
 *				the first argument of for the
 *				\b datastore_register_table().
 *	@param[in]	proc	A parser/interpreter function.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval <0	TBD.
 */
lagopus_result_t
datastore_interp_register_command(datastore_interp_t *iptr,
                                  const char *name,
                                  const char *argv0,
                                  datastore_command_proc_t proc);


/**
 * Evaluate a string.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	str	A string to be evaluated.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_eval_string(datastore_interp_t *iptr,
                             const char *name,
                             const char *str,
                             lagopus_dstring_t *result);


/**
 * Evaluate a standard I/O stream.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	infd	A standard I/O file descriptor for input.
 *	@param[in]	outfs	A standard I/O file descriptor for output.
 *	@param[in]	to_eof	Evaluate the stream until get an EOF if \b true.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_EOF		Got an EOF.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b to_eof is \b false , the interpretor
 *	evaluates only one line and returns a result. In this case, if
 *	the stream reaches the end of the file, it returns \b
 *	LAGOPUS_RESULT_EOF.
 */
lagopus_result_t
datastore_interp_eval_fd(datastore_interp_t *iptr,
                         const char *name,
                         FILE *infd,
                         FILE *outd,
                         bool to_eof,
                         lagopus_dstring_t *result);


/**
 * Evaluate a session.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	session	A session to be evaluated.
 *	@param[in]	to_eof	Evaluate the stream until get an EOF if \b true.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_EOF		Got an EOF.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b to_eof is \b false , the interpretor
 *	evaluates only one line and returns a result. In this case, if
 *	the session reaches the end of the file, it returns \b
 *	LAGOPUS_RESULT_EOF.
 */
lagopus_result_t
datastore_interp_eval_session(datastore_interp_t *iptr,
                              const char *name,
                              struct session *sptr,
                              bool to_eof,
                              lagopus_dstring_t *result);


/**
 * Evaluate a file.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	file	A file to be evaluated.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_eval_file(datastore_interp_t *iptr,
                           const char *name,
                           const char *file,
                           lagopus_dstring_t *result);

/**
 * Evaluate a file (partial).
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	file	A file to be evaluated.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_eval_file_partial(datastore_interp_t *iptr,
                                   const char *name,
                                   const char *file,
                                   lagopus_dstring_t *result);

/**
 * Load/Import a file.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	file	A file to be loaded.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details Compared to the \b datastore_interp_eval_file(), the
 *	datastore_interp_load_file() doesn't lock \b *iptr for
 *	chain/recursive dile loading. Don't use this function for
 *	top-level file evaluation.
 */
lagopus_result_t
datastore_interp_load_file(datastore_interp_t *iptr,
                           const char *name,
                           const char *file,
                           lagopus_dstring_t *result);


/**
 * Serialize all the object status and other configurations into a evaluatable string.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[out]	result	A serialized string returned.
 *
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_NOT_FOUND	Failed, database not found.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_serialize(datastore_interp_t *iptr, const char *name,
                           lagopus_dstring_t *result);


/**
 * Save all the object status and all the configurations into a file.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	name	A name of the configurater.
 *	@param[in]	file	A file to be loaded.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_OUTPUT_FAILURE	Failed, save error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_save_file(datastore_interp_t *iptr,
                           const char *name,
                           const char *file,
                           lagopus_dstring_t *result);

/**
 * Destroy all the configurations.
 *
 *	@param[in]	iptr		An interpreter.
 *	@param[in]	namespace	A namespace of the configurater.
 *	@param[out]	result		A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_destroy_obj(datastore_interp_t *iptr,
                             const char *namespace,
                             lagopus_dstring_t *result);


/**
 * Enter an atomic region.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION	Failed, invalid state transition.
 *
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_atomic_begin(datastore_interp_t *iptr,
                              const char file[],
                              lagopus_dstring_t *result);


/**
 * Abort atomic operation(s).
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION	Failed, invalid state transition.
 *
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_atomic_abort(datastore_interp_t *iptr,
                              lagopus_dstring_t *result);


/**
 * Commit atomic operation(s).
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION	Failed, invalid state transition.
 *
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_atomic_commit(datastore_interp_t *iptr,
                               lagopus_dstring_t *result);


/**
 * Rollback atomic operation(s).
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[out]	result	A result/output string.
 *	@param[in]	force	Force rollback.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION	Failed, invalid state transition.
 *
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_atomic_rollback(datastore_interp_t *iptr,
                                 lagopus_dstring_t *result,
                                 bool force);

/**
 * Enter the dry run mode.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	cur_namespace	A current namespace of the configurater.
 *	@param[in]	dup_namespace	A duplicate namespace of the configurater.
 *	@param[out]	result	A result/output string.
 *
 *	@retval LAGOPUS_RESULT_OK	Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION	Failed, invalid state transition.
 *
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_dryrun_begin(datastore_interp_t *iptr,
                              const char *cur_namespace,
                              const char *dup_namespace,
                              lagopus_dstring_t *result);


/**
 * End the dry run mode.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	namespace	A namespace of the configurater.
 *	@param[out]	result	A result/output string.
 *
 *	@retval LAGOPUS_RESULT_OK	Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_INVALID_STATE_TRANSITION	Failed, invalid state transition.
 *
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_dryrun_end(datastore_interp_t *iptr,
                            const char *namespace,
                            lagopus_dstring_t *result);

/**
 * Get the name of current configurator of the interpreter.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[out]	namep	A name of configurater returned.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This function doesn't lock the \b *iptr.
 */
lagopus_result_t
datastore_interp_get_current_configurater(const datastore_interp_t *iptr,
    const char **namep);


/**
 * Get the current file context.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[out]	filep	A filename returned (NULL allowed.)
 *	@param[out]	linep	A line # returned (NULL allowed.)
 *	@param[out]	stream_inp	A input stream returned (NULL allowed.)
 *	@param[out]	stream_outp	A output stream returned (NULL allowed.)
 *	@param[out]	gets_procp	An fgets() function returned (NULL allowed.)
 *	@param[out]	printf_procp	A printf() function returned (NULL allowed.)
 *	@param[out]	typep	A type returned (NULL allowed.)
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details The returned \b *filep and \b *linep must not be modified.
 */
lagopus_result_t
datastore_interp_get_current_file_context(const datastore_interp_t *iptr,
    const char **filep,
    size_t *linep,
    void **stream_inp,
    void **stream_outp,
    datastore_gets_proc_t *gets_procp,
    datastore_printf_proc_t
    *printf_procp,
    datastore_config_type_t *typep);


/**
 * Cleanup an internal state of a interpretor.
 *
 *	@param[in]	iptr	An interpreter.
 */
void
datastore_interp_cancel_janitor(datastore_interp_t *iptr);


/**
 * Set blocking session.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	session	A session.
 *	@param[in]	thd	A thread.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_blocking_session_set(datastore_interp_t *iptr,
                                      struct session *session,
                                      lagopus_thread_t *thd);

/**
 * Unset blocking session.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	session	A session.
 *
 *	@retval	void
 */
void
datastore_interp_blocking_session_unset(datastore_interp_t *iptr,
                                        struct session *session);


/**
 * Eval DSL cmd.
 *
 *	@param[in]	iptr	An interpreter.
 *	@param[in]	cmd	A DSL cmd string.
 *	@param[out]	result	A result/output string.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
datastore_interp_eval_cmd(datastore_interp_t *iptr,
                          const char *cmd,
                          lagopus_dstring_t *result);


/**
 * Is eval cmd.
 *
 *	@param[in]	iptr	An interpreter.
 *
 *	@retval	true/false
 */
bool
datastore_interp_is_eval_cmd(datastore_interp_t *iptr);





#endif /* ! __DATASTORE_APIS_H__ */
