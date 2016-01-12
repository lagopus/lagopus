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

#ifndef __LAGOPUS_QMUXER_H__
#define __LAGOPUS_QMUXER_H__





/**
 * @file lagopus_qmuxer.h
 */





typedef struct lagopus_cbuffer_record 	*lagopus_cbuffer_t;
#define __LAGOPUS_CBUFFER_T_DEFINED__
typedef struct lagopus_qmuxer_record *lagopus_qmuxer_t;
#define __LAGOPUS_QMUXER_T_DEFINED__
typedef lagopus_cbuffer_t lagopus_bbq_t;
#define __LAGOPUS_BBQ_T_DEFINED__


typedef struct lagopus_qmuxer_poll_record *lagopus_qmuxer_poll_t;
#define __LAGOPUS_QMUXER_POLL_T_DEFINED__


typedef enum {
  LAGOPUS_QMUXER_POLL_UNKNOWN = 0,
  LAGOPUS_QMUXER_POLL_READABLE = 0x1,
  LAGOPUS_QMUXER_POLL_WRITABLE = 0x2,
  LAGOPUS_QMUXER_POLL_BOTH = 0x3
} lagopus_qmuxer_poll_event_t;





/**
 * Create a queue muxer.
 *
 *	@param[in,out]	qmxptr	A pointer to a queue muxer to be created.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_qmuxer_create(lagopus_qmuxer_t *qmxptr);


/**
 * Destroy a queue muxer.
 *
 *	@param[in]	qmxptr	A pointer to a queue muxer to be destroyed.
 */
void
lagopus_qmuxer_destroy(lagopus_qmuxer_t *qmxptr);





/**
 * Create a polling object.
 *
 *	@param[in,out]	mpptr	A pointer to a polling onject to be created.
 *	@param[in]	bbq	A queue to be polled.
 *	@param[in]	type	A type of event to poll;
 *	\b LAGOPUS_QMUXER_POLL_READABLE ) poll the queue readable;
 *	\b LAGOPUS_QMUXER_POLL_WRITABLE ) poll the queue writable;
 *	\b LAGOPUS_QMUXER_POLL_BOTH ) poll the queue readable and/or writable.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_qmuxer_poll_create(lagopus_qmuxer_poll_t *mpptr,
                           lagopus_bbq_t bbq,
                           lagopus_qmuxer_poll_event_t type);


/**
 * Destroy a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object to be destroyed.
 */
void
lagopus_qmuxer_poll_destroy(lagopus_qmuxer_poll_t *mpptr);


/**
 * Reset internal status of a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object to be reset.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_qmuxer_poll_reset(lagopus_qmuxer_poll_t *mpptr);


/**
 * Set a queue to a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	bbq	A queue to be polled (\b NULL allowed).
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b bbq is \b NULL, the polling object is not
 *	used as an polling object and just avoided to be polled even
 *	the polling object is on the first parameter for the \b
 *	lagopus_qmuxer_poll()
 */
lagopus_result_t
lagopus_qmuxer_poll_set_queue(lagopus_qmuxer_poll_t *mpptr,
                              lagopus_bbq_t bbq);


/**
 * Get a queue from a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	bbqptr	A pointer to a queue to be returned.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details I have no guts to unify all the setter/getter APIs'
 *	parameter at this moment. Don't get me wrong.
 */
lagopus_result_t
lagopus_qmuxer_poll_get_queue(lagopus_qmuxer_poll_t *mpptr,
                              lagopus_bbq_t *bbqptr);


/**
 * Set a polling event type of a polling onject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	type	A type of event to poll;
 *	\b LAGOPUS_QMUXER_POLL_READABLE ) poll the queue readable;
 *	\b LAGOPUS_QMUXER_POLL_WRITABLE ) poll the queue writable;
 *	\b LAGOPUS_QMUXER_POLL_BOTH ) poll the queue readable and/or writable.
 *
 *	@retval LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_qmuxer_poll_set_type(lagopus_qmuxer_poll_t *mpptr,
                             lagopus_qmuxer_poll_event_t type);


/**
 * Returns # of the values in the queue of a polling ofject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_qmuxer_poll_size(lagopus_qmuxer_poll_t *mpptr);


/**
 * Returns the remaining capacity of the queue in a polling ofject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_qmuxer_poll_remaining_capacity(lagopus_qmuxer_poll_t *mpptr);


/**
 * Wait for an event on any specified poll objects.
 *
 *	@param[in]	polls	An array of pointer of poll objects.
 *	@param[in]	npolls	A # of the poll objects.
 *	@param[in]	nsec	Time to block (in nsec).
 *
 *	@retval	> 0	A # of poll objects having event.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval LAGOPUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval LAGOPUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details Note: for performance, we don't take a giant lock
 *	that blocks all the operations of queues in the poll objects
 *	when checkcing events. Because of this, there is slight
 *	possibility of dropping events. In order to avoid this, you
 *	better set appropriate timeout value in \b nsec even
 *	specifying a negative value to the \b nsec is allowed.
 *
 *	@details This API doesn't return zero/LAGOPUS_RESULT_OK.
 */
lagopus_result_t
lagopus_qmuxer_poll(lagopus_qmuxer_t *qmxptr,
                    lagopus_qmuxer_poll_t const polls[],
                    size_t npolls,
                    lagopus_chrono_t nsec);




/**
 * Cleanup an internal state of a qmuxer after thread
 * cancellation.
 *	@param[in]	qmxptr	A pointer to a qmuxer
 */
void
lagopus_qmuxer_cancel_janitor(lagopus_qmuxer_t *qmxptr);

#endif  /* ! __LAGOPUS_QMUXER_H__ */
