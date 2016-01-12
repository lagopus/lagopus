#ifndef __BASE_STAGE_H__
#define __BASE_STAGE_H__





#include "lagopus_apis.h"
#include "lagopus_pipeline_stage_internal.h"





struct base_stage_record;


typedef lagopus_result_t (*base_stage_setup_proc_t)(
    struct base_stage_record *bsptr);


typedef struct base_stage_record {
  lagopus_pipeline_stage_record m_stg;

  const char *m_basename;

  base_stage_setup_proc_t m_setup_proc;

  size_t m_n_workers;

  size_t m_n_stgs;
  size_t m_stg_idx;

  struct base_stage_record *m_next_stg;
  lagopus_chrono_t m_to;

  lagopus_bbq_t *m_qs;
  size_t m_n_qs;

  volatile size_t m_put_next_q_idx;
  volatile size_t m_get_next_q_idx;
  volatile size_t m_n_waiters;

  lagopus_mutex_t m_lock;
  lagopus_cond_t m_cond;
} base_stage_record;
typedef base_stage_record 	*base_stage_t;


typedef enum {
  base_stage_sched_unknown = 0,
  base_stage_sched_single,
  base_stage_sched_hint,
  base_stage_sched_rr,
  base_stage_sched_rr_para
} base_stage_sched_t;


typedef enum {
  base_stage_fetch_unknown = 0,
  base_stage_fetch_single,
  base_stage_fetch_hint,
  base_stage_fetch_rr
} base_stage_fetch_t;





#endif /* ! __BASE_STAGE_H__ */
