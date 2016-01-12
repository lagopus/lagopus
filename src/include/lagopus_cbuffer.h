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

#ifndef __LAGOPUS_CBUFFER_H__
#define __LAGOPUS_CBUFFER_H__





/**
 * @file lagopus_cbuffer.h
 */





#ifndef __LAGOPUS_CBUFFER_T_DEFINED__
typedef struct lagopus_cbuffer_record 	*lagopus_cbuffer_t;
#endif /* ! __LAGOPUS_CBUFFER_T_DEFINED__ */


/**
 * @details The signature of value free up functions called when
 * destroying a circular buffer.
 */
typedef void	(*lagopus_cbuffer_value_freeup_proc_t)(void **valptr);





lagopus_result_t
lagopus_cbuffer_create_with_size(lagopus_cbuffer_t *cbptr,
                                 size_t elemsize,
                                 int64_t maxelems,
                                 lagopus_cbuffer_value_freeup_proc_t proc);
/**
 * Create a circular buffer.
 *
 *     @param[in,out]	cbptr	A pointer to a circular buffer to be created.
 *     @param[in]	type	Type of the element.
 *     @param[in]	maxelems	# of maximum elements.
 *     @param[in]	proc	A value free up function (\b NULL allowed).
 *
 *     @retval LAGOPUS_RESULT_OK               Succeeded.
 *     @retval LAGOPUS_RESULT_NO_MEMORY        Failed, no memory.
 *     @retval LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
#define lagopus_cbuffer_create(cbptr, type, maxelems, proc)              \
  lagopus_cbuffer_create_with_size((cbptr), sizeof(type), (maxelems), (proc))


/**
 * Shutdown a circular buffer.
 *
 *    @param[in]	cbptr	A pointer to a circular buffer to be shutdown.
 *    @param[in]	free_values	If \b true, all the values remaining
 *    in the buffer are freed if the value free up function given by
 *    the calling of the lagopus_cbuffer_create() is not \b NULL.
 */
void
lagopus_cbuffer_shutdown(lagopus_cbuffer_t *cbptr,
                         bool free_values);


/**
 * Destroy a circular buffer.
 *
 *    @param[in]  cbptr    A pointer to a circular buffer to be destroyed.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the buffer are freed if the value free up
 *    function given by the calling of the lagopus_cbuffer_create()
 *    is not \b NULL.
 *
 *    @details if \b cbuf is operational, shutdown it.
 */
void
lagopus_cbuffer_destroy(lagopus_cbuffer_t *cbptr,
                        bool free_values);


/**
 * Clear a circular buffer.
 *
 *     @param[in]  cbptr        A pointer to a circular buffer
 *     @param[in]  free_values  If \b true, all the values
 *     remaining in the buffer are freed if the value free up
 *     function given by the calling of the lagopus_cbuffer_create()
 *     is not \b NULL.
 *
 *     @retval LAGOPUS_RESULT_OK                Succeeded.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 */
lagopus_result_t
lagopus_cbuffer_clear(lagopus_cbuffer_t *cbptr,
                      bool free_values);


/**
 * Wake up all the waiters in a circular buffer.
 *
 *     @param[in]  cbptr	A pointer to a circular buffer.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval LAGOPUS_RESULT_OK                Succeeded.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 */
lagopus_result_t
lagopus_cbuffer_wakeup(lagopus_cbuffer_t *cbptr, lagopus_chrono_t nsec);


/**
 * Wait for gettable.
 *
 *     @param[in]  cbptr	A pointer to a circular buffer.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the gettable elements.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 */
lagopus_result_t
lagopus_cbuffer_wait_gettable(lagopus_cbuffer_t *cbptr, 
                              lagopus_chrono_t nsec);


/**
 * Wait for puttable.
 *
 *     @param[in]  cbptr	A pointer to a circular buffer.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the puttable elements.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 */
lagopus_result_t
lagopus_cbuffer_wait_puttable(lagopus_cbuffer_t *cbptr, 
                              lagopus_chrono_t nsec);





lagopus_result_t
lagopus_cbuffer_put_with_size(lagopus_cbuffer_t *cbptr,
                              void **valptr,
                              size_t valsz,
                              lagopus_chrono_t nsec);
/**
 * Put an element at the tail of a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[in]  valptr     A pointer to an element.
 *     @param[in]  type       Type of a element.
 *     @param[in]  nsec       Wait time (nanosec).
 *
 *     @retval LAGOPUS_RESULT_OK                Succeeded.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 */
#define lagopus_cbuffer_put(cbptr, valptr, type, nsec)                  \
  lagopus_cbuffer_put_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                (nsec))


lagopus_result_t
lagopus_cbuffer_put_n_with_size(lagopus_cbuffer_t *cbptr,
                                void **valptr,
                                size_t n_vals,
                                size_t valsz,
                                lagopus_chrono_t nsec,
                                size_t *n_actual_put);
/**
 * Put elements at the tail of a circular buffer.
 *
 *     @param[in]  cbptr        A pointer to a circular buffer
 *     @param[in]  valptr       A pointer to elements.
 *     @param[in]  n_vals       A # of elements to put.
 *     @param[in]  type         A type of the element.
 *     @param[in]  nsec         A wait time (in nsec).
 *     @param[out] n_actual_put A pointer to a # of elements successfully put (\b NULL allowed.)
 *
 *     @retval >=0 A # of elemets to put successfully.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     elements specified by n_vals are put.
 *
 *     @details If the \b nsec is zero, it puts elements limited to a number
 *     of available rooms in the buffer (it could be zero.)
 *
 *     @details If the \b nsec is greater than zero, it puts elements limited
 *     to a number of available rooms in the buffer until the time specified
 *     by the \b nsec is expired. In this case if the actual number of
 *     elements put is less than the \b n_vals, it returns \b
 *     LAGOPUS_RESULT_TIMEDOUT.
 *
 *     @details If the \b n_actual_put is not a \b NULL, a number of elements
 *     actually put is stored.
 *
 *     @details If any errors occur while putting, it always returns an error
 *     result even if at least an element is successfully put, so check
 *     *n_actual_put if needed.
 *
 *     @details It is allowed that more then one thread simultaneously
 *     invokes this function, the atomicity of the operation is not
 *     guaranteed.
 */
#define lagopus_cbuffer_put_n(cbptr, valptr, n_vals, type, nsec,        \
                              n_actual_put)                             \
lagopus_cbuffer_put_n_with_size((cbptr), (void **)(valptr),           \
                                (n_vals), sizeof(type), (nsec),       \
                                (n_actual_put))


lagopus_result_t
lagopus_cbuffer_get_with_size(lagopus_cbuffer_t *cbptr,
                              void **valptr,
                              size_t valsz,
                              lagopus_chrono_t nsec);
/**
 * Get the head element in a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[out] valptr     A pointer to an element.
 *     @param[in]  type       A type of the element.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval LAGOPUS_RESULT_OK                Succeeded.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 */
#define lagopus_cbuffer_get(cbptr, valptr, type, nsec)                  \
  lagopus_cbuffer_get_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                (nsec))


lagopus_result_t
lagopus_cbuffer_get_n_with_size(lagopus_cbuffer_t *cbptr,
                                void **valptr,
                                size_t n_vals_max,
                                size_t n_at_least,
                                size_t valsz,
                                lagopus_chrono_t nsec,
                                size_t *n_actual_get);
/**
 * Get elements from the head of a circular buffer.
 *
 *     @param[in]  cbptr        A pointer to a circular buffer.
 *     @param[out] valptr       A pointer to a element.
 *     @param[in]  n_vals_max   A maximum # of elemetns to get.
 *     @param[in]  n_at_least   A minimum # of elements to get until timeout.
 *     @param[in]  type         A type of a element.
 *     @param[in]  nsec         A wait time (in nsec).
 *     @param[out] n_actual_get A pointer to a # of elements successfully get (\b NULL allowed.)
 *
 *     @retval >=0 A # of acuired elements.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     specified number (\b n_vals_max) of the elements are acquired.
 *
 *     @details If the \b nsec is zero, only the available elements at the
 *     moment (it could be zero) are acuqured.
 *
 *     @details If the \b nsec is greater than zero, only the elements
 *     which become available while in the time specified by the \b nsec
 *     are acquired.
 *
 *     @details If the \b nsec is greater than zero and the \b
 *     n_at_least is zero, it treats that the \b nsec is
 *     zero. Otherwise, if bothe the \b nsec and the \b n_at_least are
 *     greater than zero, it returns when the number of the elements
 *     sepcified by the \b n_at_least are acquired, even before the
 *     time period specified by the \b nsec is expired.
 *
 *     @details The \b valptr must point a sufficient size (\b sizeof(type) *
 *     \b n_vals_max) buffer enough to store elements.
 *
 *     @details If the \b n_actual_get is not a \b NULL, a number of elements
 *     actually get is stored.
 *
 *     @details If any errors occur while getting, it always returns an error
 *     result even if at least an element is successfully got, so check
 *     *n_actual_get if needed.
 *
 */
#define lagopus_cbuffer_get_n(cbptr, valptr, n_vals_max, n_at_least,    \
                              type, nsec, n_actual_get)                 \
lagopus_cbuffer_get_n_with_size((cbptr), (void **)(valptr),           \
                                (n_vals_max), (n_at_least),           \
                                sizeof(type), (nsec), (n_actual_get))


lagopus_result_t
lagopus_cbuffer_peek_with_size(lagopus_cbuffer_t *cbptr,
                               void **valptr,
                               size_t valsz,
                               lagopus_chrono_t nsec);
/**
 * Peek the head element of a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[out] valptr     A pointer to a element.
 *     @param[in]  type       A type of the element.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval LAGOPUS_RESULT_OK                Succeeded.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 */
#define lagopus_cbuffer_peek(cbptr, valptr, type, nsec)                \
  lagopus_cbuffer_peek_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                 (nsec))

lagopus_result_t
lagopus_cbuffer_peek_n_with_size(lagopus_cbuffer_t *cbptr,
                                 void **valptr,
                                 size_t n_vals_max,
                                 size_t n_at_least,
                                 size_t valsz,
                                 lagopus_chrono_t nsec,
                                 size_t *n_actual_get);
/**
 * Peek elements from the head of a circular buffer.
 *
 *     @param[in]  cbptr         A pointer to a circular buffer.
 *     @param[out] valptr        A pointer to a element.
 *     @param[in]  n_vals_max    A maximum # of elemetns to get.
 *     @param[in]  n_at_least    A minimum # of elements to get until timeout.
 *     @param[in]  type          A type of the element.
 *     @param[in]  nsec          A wait time (in nsec).
 *     @param[out] n_actual_peek A pointer to a # of elements successfully get (\b NULL allowed.)
 *
 *     @retval >=0 A # of peeked elements.
 *     @retval LAGOPUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval LAGOPUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval LAGOPUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval LAGOPUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval LAGOPUS_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     specified number (\b n_vals_max) of the elements are peeked.
 *
 *     @details If the \b nsec is zero, only the available elements at the
 *     moment (it could be zero) are peeked.
 *
 *     @details If the \b nsec is greater than zero, only the elements
 *     which become available while in the time specified by the \b nsec
 *     are peeked.
 *
 *     @details If the \b nsec is greater than zero and the \b
 *     n_at_least is zero, it treats that the \b nsec is
 *     zero. Otherwise, if bothe the \b nsec and the \b n_at_least are
 *     greater than zero, it returns when the number of the elements
 *     sepcified by the \b n_at_least are peeked, even before the
 *     time period specified by the \b nsec is expired.
 *
 *     @details The \b valptr must point a sufficient size (\b sizeof(type) *
 *     \b n_vals_max) buffer enough to store elements.
 *
 *     @details If the \b n_actual_peek is not a \b NULL, a number of elements
 *     actually get is stored.
 *
 *     @details If any errors occur while peeking, it always returns an error
 *     result even if at least an element is successfully got, so check
 *     *n_actual_peek if needed.
 */
#define lagopus_cbuffer_peek_n(cbptr, valptr, n_vals_max, n_at_least,   \
                               type, nsec, n_actual_peek)               \
lagopus_cbuffer_peek_n_with_size((cbptr), (void **)(valptr),          \
                                 (n_vals_max), (n_at_least),          \
                                 sizeof(type), (nsec), (n_actual_peek))





/**
 * Get a # of elements in a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	A # of elements in the circular buffer.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_cbuffer_size(lagopus_cbuffer_t *cbptr);


/**
 * Get the remaining capacity of a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	The remaining capacity of the circular buffer.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_cbuffer_remaining_capacity(lagopus_cbuffer_t *cbptr);


/**
 * Get the maximum capacity of a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	The maximum capacity of the circular buffer.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_cbuffer_max_capacity(lagopus_cbuffer_t *cbptr);





/**
 * Returns \b true if the circular buffer is full.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_cbuffer_is_full(lagopus_cbuffer_t *cbptr, bool *retptr);


/**
 * Returns \b true if the circular buffer is empty.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_cbuffer_is_empty(lagopus_cbuffer_t *cbptr, bool *retptr);


/**
 * Returns \b true if the circular buffer is operational.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_cbuffer_is_operational(lagopus_cbuffer_t *cbptr, bool *retptr);


/**
 * Cleanup an internal state of a circular buffer after thread
 * cancellation.
 *	@param[in]	cbptr	A pointer to a circular buffer
 */
void
lagopus_cbuffer_cancel_janitor(lagopus_cbuffer_t *cbptr);





#endif  /* ! __LAGOPUS_CBUFFER_H__ */
