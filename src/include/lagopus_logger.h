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

#ifndef __LAGOPUS_LOGGER_H__
#define __LAGOPUS_LOGGER_H__





/**
 * @file	lagopus_logger.h
 */





#include "openflow.h"
#include "openflow13.h"
#include "lagopus/ofp_pdump.h"




#define TRACE_OFPT_HELLO 1
#define TRACE_OFPT_ERROR (1 << (int)OFPT_ERROR)
#define TRACE_OFPT_ECHO_REQUEST (1 << (int)OFPT_ECHO_REQUEST)
#define TRACE_OFPT_ECHO_REPLY (1 << (int)OFPT_ECHO_REPLY)
#define TRACE_OFPT_EXPERIMENTER (1 << (int)OFPT_EXPERIMENTER)
#define TRACE_OFPT_FEATURES_REQUEST (1 << (int)OFPT_FEATURES_REQUEST)
#define TRACE_OFPT_FEATURES_REPLY (1 << (int)OFPT_FEATURES_REPLY)
#define TRACE_OFPT_GET_CONFIG_REQUEST (1 << (int)OFPT_GET_CONFIG_REQUEST)
#define TRACE_OFPT_GET_CONFIG_REPLY (1 << (int)OFPT_GET_CONFIG_REPLY)
#define TRACE_OFPT_SET_CONFIG (1 << (int)OFPT_SET_CONFIG)
#define TRACE_OFPT_PACKET_IN (1 << (int)OFPT_PACKET_IN)
#define TRACE_OFPT_FLOW_REMOVED (1 << (int)OFPT_FLOW_REMOVED)
#define TRACE_OFPT_PORT_STATUS (1 << (int)OFPT_PORT_STATUS)
#define TRACE_OFPT_PACKET_OUT (1 << (int)OFPT_PACKET_OUT)
#define TRACE_OFPT_FLOW_MOD (1 << (int)OFPT_FLOW_MOD)
#define TRACE_OFPT_GROUP_MOD (1 << (int)OFPT_GROUP_MOD)
#define TRACE_OFPT_PORT_MOD (1 << (int)OFPT_PORT_MOD)
#define TRACE_OFPT_TABLE_MOD (1 << (int)OFPT_TABLE_MOD)
#define TRACE_OFPT_MULTIPART_REQUEST (1 << (int)OFPT_MULTIPART_REQUEST)
#define TRACE_OFPT_MULTIPART_REPLY (1 << (int)OFPT_MULTIPART_REPLY)
#define TRACE_OFPT_BARRIER_REQUEST (1 << (int)OFPT_BARRIER_REQUEST)
#define TRACE_OFPT_BARRIER_REPLY (1 << (int)OFPT_BARRIER_REPLY)
#define TRACE_OFPT_QUEUE_GET_CONFIG_REQUEST (1 << (int)OFPT_QUEUE_GET_CONFIG_REQUEST)
#define TRACE_OFPT_QUEUE_GET_CONFIG_REPLY (1 << (int)OFPT_QUEUE_GET_CONFIG_REPLY)
#define TRACE_OFPT_ROLE_REQUEST (1 << (int)OFPT_ROLE_REQUEST)
#define TRACE_OFPT_ROLE_REPLY (1 << (int)OFPT_ROLE_REPLY)
#define TRACE_OFPT_GET_ASYNC_REQUEST (1 << (int)OFPT_GET_ASYNC_REQUEST)
#define TRACE_OFPT_GET_ASYNC_REPLY (1 << (int)OFPT_GET_ASYNC_REPLY)
#define TRACE_OFPT_SET_ASYNC (1 << (int)OFPT_SET_ASYNC)
#define TRACE_OFPT_METER_MOD (1 << (int)OFPT_METER_MOD)

#define TRACE_FULL 0xffffffff

#define TRACE_MAX_SIZE 4096




typedef enum {
  LAGOPUS_LOG_LEVEL_UNKNOWN = 0,
  LAGOPUS_LOG_LEVEL_DEBUG,
  LAGOPUS_LOG_LEVEL_TRACE,
  LAGOPUS_LOG_LEVEL_INFO,
  LAGOPUS_LOG_LEVEL_NOTICE,
  LAGOPUS_LOG_LEVEL_WARNING,
  LAGOPUS_LOG_LEVEL_ERROR,
  LAGOPUS_LOG_LEVEL_FATAL
} lagopus_log_level_t;


typedef enum {
  LAGOPUS_LOG_EMIT_TO_UNKNOWN = 0,
  LAGOPUS_LOG_EMIT_TO_FILE,
  LAGOPUS_LOG_EMIT_TO_SYSLOG
} lagopus_log_destination_t;





/**
 * Initialize the logger.
 *
 *	@param[in]	dst	Where to log;
 *	\b LAGOPUS_LOG_EMIT_TO_UNKNOWN: stderr,
 *	\b LAGOPUS_LOG_EMIT_TO_FILE: Any regular file,
 *	\b LAGOPUS_LOG_EMIT_TO_SYSLOG: syslog
 *	@param[in]	arg	For \b LAGOPUS_LOG_EMIT_TO_FILE: a file name,
 *	for \b LAGOPUS_LOG_EMIT_TO_SYSLOG: An identifier for syslog.
 *	@param[in]	multi_process	If the \b dst is
 *	\b LAGOPUS_LOG_EMIT_TO_FILE, use \b true if the application shares
 *	the log file between child processes.
 *	@param[in]	emit_date	Use \b true if date is needed in each
 *	line header.
 *	@param[in]	debug_level	A debug level.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
lagopus_log_initialize(lagopus_log_destination_t dst,
                       const char *arg,
                       bool multi_process,
                       bool emit_date,
                       uint16_t debug_level);


/**
 * Re-initialize the logger.
 *
 *	@details Calling this function implies 1) close opened log
 *	file. 2) re-open the log file, convenient for the log rotation.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t	lagopus_log_reinitialize(void);


/**
 * Finalize the logger.
 */
void	lagopus_log_finalize(void);


/**
 * Set the debug level.
 *
 *	@param[in]	lvl	A debug level.
 */
void	lagopus_log_set_debug_level(uint16_t lvl);


/**
 * Get the debug level.
 *
 *	@returns	The debug level.
 */
uint16_t	lagopus_log_get_debug_level(void);


/**
 * Set the trace flags.
 *
 *	@param[in]	flags	trace flags. Or'd value of TRACE_OFPT_*.
 */
void	lagopus_log_set_trace_flags(uint32_t flags);


/**
 * Unset the trace flags.
 *
 *	@param[in]	flags	trace flags. Or'd value of TRACE_OFPT_*.
 */
void	lagopus_log_unset_trace_flags(uint32_t flags);


/**
 * Get the trace flags.
 *
 *	@returns The trace flags.
 */
uint32_t	lagopus_log_get_trace_flags(void);


/**
 * Set the trace packet flag.
 *
 *	@param[in]	flag	trace packet flags(true or false).
 */
void
lagopus_log_set_trace_packet_flag(bool flag);


/**
 * Check the trace flags.
 *
 *	@returns Enable or disable the trace flags.
 */
bool	lagopus_log_check_trace_flags(uint64_t flags);


/**
 * Check where the log is emitted to.
 *
 *	@param[out]	A pointer to the argument that was passed via the \b lagopus_log_get_destination(). (NULL allowed.)
 *
 *	@returns The log destination.
 *
 *	@details If the \b arg is specified as non-NULL pointer, the
 *	returned \b *arg must not be free()'d nor modified.
 */
lagopus_log_destination_t
lagopus_log_get_destination(const char **arg);


/**
 * The main logging workhorse: not intended for direct use.
 */
void	lagopus_log_emit(lagopus_log_level_t log_level,
                       uint64_t debug_level,
                       const char *file,
                       int line,
                       const char *func,
                       const char *fmt, ...)
__attr_format_printf__(6, 7);





#ifdef __GNUC__
#define __PROC__	__PRETTY_FUNCTION__
#else
#define	__PROC__	__func__
#endif /* __GNUC__ */


/**
 * Emit a debug message to the log.
 *
 *	@param[in]	level	A debug level (int).
 */
#define lagopus_msg_debug(level, ...) \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_DEBUG, (uint64_t)(level), \
                   __FILE__, __LINE__, __PROC__, __VA_ARGS__)


/**
 * Emit a trace message to the log.
 *
 *	@param[in]	flags	Or'd value of TRACE_OFPT_*.
 *	@param[in]	detail	A boolean for detailed trace logging.
 */
#define lagopus_msg_trace(flags, detail, ...)                           \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_TRACE,                             \
                   ((uint64_t)(flags) |                                 \
                    ((detail == true) ? (uint64_t)(1LL << 32) : 0LL)),  \
                   __FILE__, __LINE__, __PROC__, __VA_ARGS__)


/**
 * Emit a packet dump to the log.
 *
 *	@param[in]	flags	Or'd value of TRACE_OFPT_*.
 *	@param[in]	type	Type of pdump (PDUMP_OFP_*).
 *	@param[in]	packet	A ofp_* variable name.
 *	@param[in]	fmt	Format of output.
 */
#define lagopus_msg_pdump(flags, type, packet, fmt, ...) {              \
    char __tbuf[TRACE_MAX_SIZE];                                        \
    (void) trace_call(type, (void *) &(packet),                         \
                      sizeof(packet),                                   \
                      __tbuf, TRACE_MAX_SIZE);                          \
    lagopus_log_emit(LAGOPUS_LOG_LEVEL_TRACE, (uint64_t) (flags),       \
                     __FILE__, __LINE__,__PROC__,                       \
                     fmt "%s\n" , ##__VA_ARGS__, __tbuf);               \
  }


/**
 * Emit an informative message to the log.
 */
#define lagopus_msg_info(...) \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_INFO, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit a notice message to the log.
 */
#define lagopus_msg_notice(...) \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_NOTICE, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit a warning message to the log.
 */
#define lagopus_msg_warning(...) \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_WARNING, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit an error message to the log.
 */
#define lagopus_msg_error(...) \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_ERROR, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit a fatal message to the log.
 */
#define lagopus_msg_fatal(...) \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_FATAL, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit an arbitarary message to the log.
 */
#define lagopus_msg(...) \
  lagopus_log_emit(LAGOPUS_LOG_LEVEL_UNKNOWN, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * The minimum level debug emitter.
 */
#define lagopus_dprint(...) \
  lagopus_msg_debug(1LL, __VA_ARGS__)


/**
 * Emit a readable error message for the errornous result.
 *
 *	@param[in]	s	A result code (lagopus_result_t)
 */
#define lagopus_perror(s) \
  do {                                                                  \
    (s == LAGOPUS_RESULT_POSIX_API_ERROR) ?                             \
    lagopus_msg_error("LAGOPUS_RESULT_POSIX_API_ERROR: %s.\n",      \
                      strerror(errno)) :                            \
    lagopus_msg_error("%s.\n", lagopus_error_get_string((s)));      \
  } while (0)


/**
 * Emit an error message and exit.
 *
 *	@param[in]	ecode	An exit code (int)
 */
#define lagopus_exit_error(ecode, ...) {        \
    lagopus_msg_error(__VA_ARGS__);             \
    exit(ecode);                                \
  }


/**
 * Emit a fatal message and abort.
 */
#define lagopus_exit_fatal(...) {                   \
    lagopus_msg_fatal(__VA_ARGS__);                 \
    abort();                                        \
  }





#endif /* ! __LAGOPUS_LOGGER_H__ */
