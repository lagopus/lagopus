#ifndef __LEGACY_SW_STAGE_H__
#define __LEGACY_SW_STAGE_H__





#include "lagopus_apis.h"
#include "lagopus_pipeline_stage_internal.h"





typedef struct legacy_sw_stage_record {
  base_stage_record m_base_stg;

  lagopus_mutex_t m_lock;
  lagopus_cond_t m_cond;
  volatile bool m_do_exit;
} legacy_sw_stage_record;
typedef legacy_sw_stage_record	*legacy_sw_stage_t;


typedef struct legacy_sw_stage_spec_t {
  size_t m_n_workers;
  size_t m_n_qs;
  size_t m_q_len;
  size_t m_batch_size;
  base_stage_sched_t m_sched_type;
  base_stage_fetch_t m_fetch_type;
  lagopus_pipeline_stage_main_proc_t main_proc;
  lagopus_chrono_t m_to;
} legacy_sw_stage_spec_t;





#endif /* ! __LEGACY_SW_STAGE_H__ */
