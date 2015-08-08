/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#ifndef __LAGOPUS_CALLOUT_INTERNAL_H__
#define __LAGOPUS_CALLOUT_INTERNAL_H__






/**
 *	@file	lagopus_callout_internal.h
 */





#include "lagopus_callout_task_funcs.h"
#include "lagopus_runnable_internal.h"





/**
 * The callout task status.
 */
typedef enum {
  TASK_STATE_UNKNOWN = 0,
  TASK_STATE_CREATED,	/** Created. */
  TASK_STATE_ENQUEUED,	/** Submitted/enqueued. */
  TASK_STATE_DEQUEUED,	/** Decqueud for execution. */
  TASK_STATE_EXECUTING,	/** Just executing at the moment. */
  TASK_STATE_EXECUTED,	/** Executed. */
  TASK_STATE_EXEC_FAILED,	/** Execution failed. */
  TASK_STATE_CANCELLED,	/** Cancelled. */
  TASK_STATE_DELETING	/** Just deleting at the moment. */
} callout_task_state_t;


/**
 * The callout task type.
 */
typedef enum {
  TASK_TYPE_UNKNOWN,
  TASK_TYPE_SINGLE_SHOT,
  TASK_TYPE_PERIODIC
} callout_task_type_t;


typedef struct lagopus_callout_task_record {
  lagopus_runnable_record m_runnable;
  TAILQ_ENTRY(lagopus_callout_task_record) m_entry;

  lagopus_mutex_t m_lock;

  callout_task_type_t m_type;
  const char *m_name;
  lagopus_callout_task_proc_t m_proc;
  lagopus_callout_task_get_event_n_proc_t m_get_n_proc;
  void *m_arg;
  lagopus_callout_task_arg_freeup_proc_t m_freeproc;
  bool m_do_repeat;
  bool m_is_first;
  bool m_is_in_q;	/** We need this since the TAILQ_* APIs don't
                            check that a specified entry is really in
                            a specified queue for the
                            insertion/removal. */
  lagopus_chrono_t m_initial_delay_time;
  lagopus_chrono_t m_interval_time;
  lagopus_chrono_t m_last_abstime;
  lagopus_chrono_t m_next_abstime;
} lagopus_callout_task_record;





#endif /* ! __LAGOPUS_CALLOUT_INTERNAL_H__ */
