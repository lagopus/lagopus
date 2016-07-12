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

/**
 * @file        legacy_sw_pipeline.h
 * @brief       Pipeline header of legacy switch functionality.
 */


#ifndef SRC_INCLUDE_LEGACY_SW_PIPELINE_H_
#define SRC_INCLUDE_LEGACY_SW_PIPELINE_H_


#include "stdint.h"
#include "stdbool.h"

#include "lagopus_pipeline_stage.h"


enum pipeline_index {
  L2_PIPELINE = 0,
  L3_PIPELINE,
  MAX_PIPELINE
};


struct pipeline_context {
  bool                  error;
  uint8_t               is_last_packet_of_bulk;
  uint32_t              output_port;
  enum pipeline_index   pipeline_idx;
};


typedef void
(*lagopus_dp_pipeline_main_proc_t)(struct lagopus_packet *packet);


struct pipeline_stage_layout {
  lagopus_dp_pipeline_main_proc_t main_proc;
  size_t                          n_workers;
  uint32_t                        worker_id_offset;
};


#define N_MAX_STAGES	4
struct pipeline_layout {
  const char                   name[64];
  size_t                       n_stages;
  struct pipeline_stage_layout stages[N_MAX_STAGES];
};


#endif /* SRC_INCLUDE_LEGACY_SW_PIPELINE_H_ */
