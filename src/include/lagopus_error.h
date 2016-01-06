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

#ifndef __LAGOPUS_ERROR_H__
#define __LAGOPUS_ERROR_H__





/**
 *	@file	lagopus_error.h
 */





#define LAGOPUS_RESULT_OK		0


/*
 * Note:
 *
 *	Add error #s after this and add messages that corresponds to
 *	the error #s into s_error_strs[] in lib/error.c, IN ORDER.
 */


#define LAGOPUS_RESULT_ANY_FAILURES	-1
#define LAGOPUS_RESULT_POSIX_API_ERROR	-2
#define LAGOPUS_RESULT_NO_MEMORY	-3
#define LAGOPUS_RESULT_NOT_FOUND	-4
#define LAGOPUS_RESULT_ALREADY_EXISTS	-5
#define LAGOPUS_RESULT_NOT_OPERATIONAL	-6
#define LAGOPUS_RESULT_INVALID_ARGS	-7
#define LAGOPUS_RESULT_NOT_OWNER	-8
#define LAGOPUS_RESULT_NOT_STARTED	-9
#define LAGOPUS_RESULT_TIMEDOUT		-10
#define LAGOPUS_RESULT_ITERATION_HALTED	-11
#define LAGOPUS_RESULT_OUT_OF_RANGE	-12
#define LAGOPUS_RESULT_NAN		-13
#define LAGOPUS_RESULT_OFP_ERROR	-14
#define LAGOPUS_RESULT_ALREADY_HALTED	-15
#define LAGOPUS_RESULT_INVALID_OBJECT	-16
#define LAGOPUS_RESULT_CRITICAL_REGION_NOT_CLOSED	-17
#define LAGOPUS_RESULT_CRITICAL_REGION_NOT_OPENED	-18
#define LAGOPUS_RESULT_INVALID_STATE_TRANSITION		-19
#define LAGOPUS_RESULT_SOCKET_ERROR		-20
#define LAGOPUS_RESULT_BUSY		-21
#define LAGOPUS_RESULT_STOP		-22
#define LAGOPUS_RESULT_SNMP_API_ERROR		-23
#define LAGOPUS_RESULT_TLS_CONN_ERROR		-24
#define LAGOPUS_RESULT_EINPROGRESS		-25
#define LAGOPUS_RESULT_OXM_ERROR		-26
#define LAGOPUS_RESULT_UNSUPPORTED	-27
#define LAGOPUS_RESULT_QUOTE_NOT_CLOSED		-28
#define LAGOPUS_RESULT_NOT_ALLOWED		-29
#define LAGOPUS_RESULT_NOT_DEFINED	-30
#define LAGOPUS_RESULT_WAKEUP_REQUESTED	-31
#define LAGOPUS_RESULT_TOO_MANY_OBJECTS	-32
#define LAGOPUS_RESULT_DATASTORE_INTERP_ERROR	-33
#define LAGOPUS_RESULT_EOF		-34
#define LAGOPUS_RESULT_NO_MORE_ACTION		-35
#define LAGOPUS_RESULT_TOO_LARGE	-36
#define LAGOPUS_RESULT_TOO_SMALL	-37
#define LAGOPUS_RESULT_TOO_LONG		-38
#define LAGOPUS_RESULT_TOO_SHORT	-39
#define LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE		-40
#define LAGOPUS_RESULT_OUTPUT_FAILURE	-41
#define LAGOPUS_RESULT_INVALID_STATE	-42
#define LAGOPUS_RESULT_INVALID_NAMESPACE	-43
#define LAGOPUS_RESULT_INTERRUPTED	-44





/**
 * Get a human readable error message from an API result code.
 *
 *	@param[in]	err	A result code.
 *
 *	@returns	A human readable error message.
 */
const char 	*lagopus_error_get_string(lagopus_result_t err);





#endif /* ! __LAGOPUS_ERROR_H__ */
