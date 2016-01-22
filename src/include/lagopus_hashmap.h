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

#ifndef __LAGOPUS_HASHMAP_H__
#define __LAGOPUS_HASHMAP_H__





/**
 * @file	lagopus_hashmap.h
 */





#define LAGOPUS_HASHMAP_TYPE_STRING	0U
#define LAGOPUS_HASHMAP_TYPE_ONE_WORD \
  MACRO_CONSTANTIFY_UNSIGNED(SIZEOF_VOID_P)





typedef unsigned int lagopus_hashmap_type_t;


typedef struct HashEntry 	*lagopus_hashentry_t;
typedef struct lagopus_hashmap_record 	*lagopus_hashmap_t;


/**
 * @details The signature of hash map iteration functions. Returning
 * \b false stops the iteration.
 */
typedef bool
(*lagopus_hashmap_iteration_proc_t)(void *key, void *val,
                                    lagopus_hashentry_t he,
                                    void *arg);


/**
 * @details The signature of value free up functions called when
 * destroying hash maps.
 */
typedef void
(*lagopus_hashmap_value_freeup_proc_t)(void *value);





/**
 * Set a value to an entry.
 *
 *	@param[in]	he	A hash entry.
 *	@param[in]	val	A value.
 *
 *	@details Using this function is recommended only in iteration
 *	functions invoked via the lagopus_hashmap_iterate().
 */
void
lagopus_hashmap_set_value(lagopus_hashentry_t he,
                          void *val);


/**
 * Create a hash map.
 *
 *	@param[out]	retptr	A pointer to a hash map to be created.
 *	@param[in]	t	The type of key
 *	(\b LAGOPUS_HASHMAP_TYPE_STRING: string key,
 *	\b LAGOPUS_HASHMAP_TYPE_ONE_WORD: 64/32 bits unsigned integer key)
 *	@param[in]	proc	A value free up function (\b NULL allowed).
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details The \b proc is called when lagopus_hashmap_destroy()
 *	is called, for each stored value in the hash map to free it
 *	up, if both the \b proc and the value are not \b NULL.
 *
 *	@details And note that you can use arbitrary key, by
 *	specifying \b t as an integer, in \b sizeof(int). And in this
 *	case, the key itself must be passed as a pointer for the key.
 *
 *	@details So if you want to use 64 bits key on 32 bits
 *	architecture, you must pass the \b t as
 *	\b (lagopus_hashmap_type_t)(sizeof(int64_t) / sizeof(int))
 */
lagopus_result_t
lagopus_hashmap_create(lagopus_hashmap_t *retptr,
                       lagopus_hashmap_type_t t,
                       lagopus_hashmap_value_freeup_proc_t proc);


/**
 * Shutdown a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the lagopus_hashmap_create()
 *	is not \b NULL.
 *
 *	@details Call this function for the graceful hash map deletion
 *	of multi-threaded applications. Call this before calling the
 *	lagopus_hashmap_destroy() makes the hash map not operational
 *	anymore so that any other threads could notice that the hash
 *	map is not usable.
 */
void
lagopus_hashmap_shutdown(lagopus_hashmap_t *hmptr,
                         bool free_values);


/**
 * Destroy a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the lagopus_hashmap_create()
 *	is not \b NULL.
 *
 *	@details Calling this function would imply invocation of the
 *	lagopus_hashmap_shutdown() if needed.
 */
void
lagopus_hashmap_destroy(lagopus_hashmap_t *hmptr,
                        bool free_values);


/**
 * Clear a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the lagopus_hashmap_create()
 *	is not \b NULL.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_hashmap_clear(lagopus_hashmap_t *hmptr,
                      bool free_values);


/**
 * Clear a hash map (no lock).
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the lagopus_hashmap_create()
 *	is not \b NULL.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_hashmap_clear_no_lock(lagopus_hashmap_t *hmptr,
                              bool free_values);


/**
 * Find a value corresponding to a given key from a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to a value to be found.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded, found the value.
 *	@retval LAGOPUS_RESULT_NOT_FOUND	Failed, the value not found.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_hashmap_find(lagopus_hashmap_t *hmptr,
                     void *key,
                     void **valptr);


/**
 * Find a value corresponding to a given key from a hash map (no lock).
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to a value to be found.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded, found the value.
 *	@retval LAGOPUS_RESULT_NOT_FOUND	Failed, the value not found.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This version of the find API doesn't take a reader lock.
 */
lagopus_result_t
lagopus_hashmap_find_no_lock(lagopus_hashmap_t *hmptr,
                             void *key,
                             void **valptr);


/**
 * Add a key - value pair to a hash map.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	key	A key.
 *	@param[in,out]	valptr	A pointr to a value.
 *	@param[in]	allow_overwrite When the pair already exists; \b true:
 *	overwrite, \b false: the operation is canceled.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded, the value newly
 *	added.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ALREADY_EXISTS	Failed, the key - value pair
 *	already exists.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details No matter what the \b allow_overwrite is, if a value
 *	for the \b key doesn't exist in the hash map and the function
 *	returns \b LAGOPUS_RESULT_OK, the \b *valptr is set to \b NULL
 *	to indicate that no entry confliction occurs while the
 *	addition. And in this case, it is not possible to distinguish
 *	the cases between; 1) the entry doesn't exist. 2) the entry
 *	does exists and the value is \b NULL. If it is needed to
 *	distinguish these cases (e.g.: to add the \b NULL to a hash
 *	map as a value is very nature of the application), call the
 *	lagopus_hashmap_find() to check the existence of the entry
 *	before the addition.
 *
 *	@details And also no matter what the \b allow_overwrite is, if
 *	an entry exists for the \b key, the returned \b *valptr is the
 *	former value (including \b NULL). And also if the functions
 *	returns \b LAGOPUS_RESULT_OK (note that the \b allow_overwrite
 *	is \b true for this case), it could be needed to free the
 *	value up.
 *
 *	@details If the \b allow_overwrite is \b false and the
 *	functions returns \b LAGOPUS_RESULT_ALREADY_EXISTS, nothing is
 *	changed in the hash map and the \b *valptr is the current
 *	value.
 *
 *	@details <em> And, NOTE THAT the \b *valptr could be modified
 *	almost always. If it is needed to keep the value referred by
 *	the \b valptr, backup/copy it before calling this function or
 *	pass the copied pointer. </em>
 */
lagopus_result_t
lagopus_hashmap_add(lagopus_hashmap_t *hmptr,
                    void *key,
                    void **valptr,
                    bool allow_overwrite);


/**
 * Add a key - value pair to a hash map (no lock).
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	key	A key.
 *	@param[in,out]	valptr	A pointr to a value.
 *	@param[in]	allow_overwrite When the pair already exists; \b true:
 *	overwrite, \b false: the operation is canceled.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded, the value newly
 *	added.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ALREADY_EXISTS	Failed, the key - value pair
 *	already exists.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details No matter what the \b allow_overwrite is, if a value
 *	for the \b key doesn't exist in the hash map and the function
 *	returns \b LAGOPUS_RESULT_OK, the \b *valptr is set to \b NULL
 *	to indicate that no entry confliction occurs while the
 *	addition. And in this case, it is not possible to distinguish
 *	the cases between; 1) the entry doesn't exist. 2) the entry
 *	does exists and the value is \b NULL. If it is needed to
 *	distinguish these cases (e.g.: to add the \b NULL to a hash
 *	map as a value is very nature of the application), call the
 *	lagopus_hashmap_find() to check the existence of the entry
 *	before the addition.
 *
 *	@details And also no matter what the \b allow_overwrite is, if
 *	an entry exists for the \b key, the returned \b *valptr is the
 *	former value (including \b NULL). And also if the functions
 *	returns \b LAGOPUS_RESULT_OK (note that the \b allow_overwrite
 *	is \b true for this case), it could be needed to free the
 *	value up.
 *
 *	@details If the \b allow_overwrite is \b false and the
 *	functions returns \b LAGOPUS_RESULT_ALREADY_EXISTS, nothing is
 *	changed in the hash map and the \b *valptr is the current
 *	value.
 *
 *	@details <em> And, NOTE THAT the \b *valptr could be modified
 *	almost always. If it is needed to keep the value referred by
 *	the \b valptr, backup/copy it before calling this function or
 *	pass the copied pointer. </em>
 *
 *	@details This version of the add API doesn't take a writer lock.
 */
lagopus_result_t
lagopus_hashmap_add_no_lock(lagopus_hashmap_t *hmptr,
                            void *key,
                            void **valptr,
                            bool allow_overwrite);


/**
 * Delete a key - value pair specified by the key.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to save the former value
 *	if it exists (\b NULL allowed).
 *	@param[in]	free_value	If \b true, the value corresponding
 *	to the key is freed up if the free up function given by the
 *	calling of the lagopus_hashmap_create() is not \b NULL.
 *
 *	@retval LAGOPUS_RESULT_OK Succeeded, the pair deleted, or the
 *	pair doesn't exist.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b free_value is \b true, <em> the returned \b
 *	*valptr is \b NOT accessible EVEN it is not \b
 *	NULL. </em>
 */
lagopus_result_t
lagopus_hashmap_delete(lagopus_hashmap_t *hmptr,
                       void *key,
                       void **valptr,
                       bool free_value);


/**
 * Delete a key - value pair specified by the key (no lock).
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to save the former value
 *	if it exists (\b NULL allowed).
 *	@param[in]	free_value	If \b true, the value corresponding
 *	to the key is freed up if the free up function given by the
 *	calling of the lagopus_hashmap_create() is not \b NULL.
 *
 *	@retval LAGOPUS_RESULT_OK Succeeded, the pair deleted, or the
 *	pair doesn't exist.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b free_value is \b true, <em> the returned \b
 *	*valptr is \b NOT accessible EVEN it is not \b
 *	NULL. </em>
 *
 *	@details This version of the delete API doesn't take a writer lock.
 */
lagopus_result_t
lagopus_hashmap_delete_no_lock(lagopus_hashmap_t *hmptr,
                               void *key,
                               void **valptr,
                               bool free_value);


/**
 * Get a # of entries in a hash map.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *
 *	@retval	>=0	A # of entries (a # of pairs) in the hash map.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_hashmap_size(lagopus_hashmap_t *hmptr);


/**
 * Get a # of entries in a hash map (no lock).
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *
 *	@retval	>=0	A # of entries (a # of pairs) in the hash map.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_hashmap_size_no_lock(lagopus_hashmap_t *hmptr);


/**
 * Apply a function to all entries in a hash map iteratively.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	proc	An iteration function.
 *	@param[in]	arg	An auxiliary argument for the \b proc
 *	(\b NULL allowed).
 *
 *	@details DO NOT add/delete any entries in any iteration
 *	functions. The modification of the values of entries by
 *	calling the lagopus_hashmap_set_value() is allowed in the
 *	functions.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_ITERATION_HALTED The iteration was
 *	stopped since the \b proc returned \b false.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b proc returns false while the iteration, the
 *	iteration stops.
 */
lagopus_result_t
lagopus_hashmap_iterate(lagopus_hashmap_t *hmptr,
                        lagopus_hashmap_iteration_proc_t proc,
                        void *arg);


/**
 * Apply a function to all entries in a hash map iteratively (no lock).
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	proc	An iteration function.
 *	@param[in]	arg	An auxiliary argument for the \b proc
 *	(\b NULL allowed).
 *
 *	@details DO NOT add/delete any entries in any iteration
 *	functions. The modification of the values of entries by
 *	calling the lagopus_hashmap_set_value() is allowed in the
 *	functions.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_ITERATION_HALTED The iteration was
 *	stopped since the \b proc returned \b false.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b proc returns false while the iteration, the
 *	iteration stops.
 *
 *	@details This version of the iterate API doesn't take a writer lock.
 */
lagopus_result_t
lagopus_hashmap_iterate_no_lock(lagopus_hashmap_t *hmptr,
                                lagopus_hashmap_iteration_proc_t proc,
                                void *arg);


/**
 * Get statistics of a hash map.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[out]	msgptr	A pointer to a string including statistics.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the returned \b *msgptr is not \b NULL, it must be
 *	freed up by \b free().
 */
lagopus_result_t
lagopus_hashmap_statistics(lagopus_hashmap_t *hmptr,
                           const char **msgptr);


/**
 * Preparation for fork(2).
 *
 *	@details This function is provided for the pthread_atfork(3)'s
 *	last argument. If we have to, functions for the first and
 *	second arguments for the pthread_atfork(3) would be provided
 *	as well, in later.
 */
void
lagopus_hashmap_atfork_child(lagopus_hashmap_t *hmptr);





#endif /* ! __LAGOPUS_HASHMAP_H__ */
