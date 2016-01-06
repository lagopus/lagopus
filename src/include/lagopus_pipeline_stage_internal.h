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

#ifndef __LAGOPUS_PIPELINE_STAGE_INTERNAL_H__
#define __LAGOPUS_PIPELINE_STAGE_INTERNAL_H__





#include "lagopus_pipeline_stage_funcs.h"





typedef struct lagopus_pipeline_worker_record 	*lagopus_pipeline_worker_t;


typedef enum {
  STAGE_STATE_UNKNOWN = 0,
  STAGE_STATE_INITIALIZED,
  STAGE_STATE_SETUP,
  STAGE_STATE_STARTED,
  STAGE_STATE_PAUSED,
  STAGE_STATE_MAINTENANCE_REQUESTED,
  STAGE_STATE_CANCELED,
  STAGE_STATE_SHUTDOWN,
  STAGE_STATE_FINALIZED,
  STAGE_STATE_DESTROYING
} lagopus_pipeline_stage_state_t;


typedef struct lagopus_pipeline_stage_record {
  lagopus_pipeline_stage_pre_pause_proc_t m_pre_pause_proc;
  lagopus_pipeline_stage_sched_proc_t m_sched_proc;

  lagopus_pipeline_stage_setup_proc_t m_setup_proc;
  lagopus_pipeline_stage_fetch_proc_t m_fetch_proc;
  lagopus_pipeline_stage_main_proc_t m_main_proc;
  lagopus_pipeline_stage_throw_proc_t m_throw_proc;
  lagopus_pipeline_stage_shutdown_proc_t m_shutdown_proc;
  lagopus_pipeline_stage_finalize_proc_t m_final_proc;
  lagopus_pipeline_stage_freeup_proc_t m_freeup_proc;

  const char *m_name;

  lagopus_pipeline_worker_t *m_workers;
  size_t m_n_workers;

  size_t m_event_size;
  size_t m_max_batch;
  size_t m_batch_buffer_size;	/* == m_event_size * m_max_batch (in bytes.) */

  bool m_is_heap_allocd;

  lagopus_mutex_t m_lock;
  lagopus_cond_t m_cond;

  volatile bool m_do_loop;
  volatile shutdown_grace_level_t m_sg_lvl;

  volatile lagopus_pipeline_stage_state_t m_status;

  lagopus_mutex_t m_final_lock;
  size_t m_n_canceled_workers;
  size_t m_n_shutdown_workers;

  volatile bool m_pause_requested;
  lagopus_barrier_t m_pause_barrier;
  lagopus_mutex_t m_pause_lock;
  lagopus_cond_t m_pause_cond;
  lagopus_cond_t m_resume_cond;

  lagopus_pipeline_stage_maintenance_proc_t m_maint_proc;
  void *m_maint_arg;

  lagopus_pipeline_stage_post_start_proc_t m_post_start_proc;
  void *m_post_start_arg;
} lagopus_pipeline_stage_record;





#endif /* __LAGOPUS_PIPELINE_STAGE_INTERNAL_H__ */
