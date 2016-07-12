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
 *      @file   pipeline.c
 *      @brief  Legacy L2/L3 dataplane with pipeliner.
 */

#ifdef HYBRID

#include <stdint.h>

#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/interface.h"
#include "lagopus/dataplane.h"
#include "lagopus/pipeline.h"


#include "lagopus/ethertype.h"
#include "pktbuf.h"
#include "packet.h"
#include "pipeline.h"





#include "pipeline_base_stage.h"
#include "pipeline_stage.h"





#include "pipeline_base_stage.c"
#include "pipeline_stage.c"





#include "lagopus/mactable.h"




/* #define PERFORMANCE_TEST */

#define N_PIPELINES             2 /* currently, we have l2 and l3 pipelines */
#define N_L2_STAGES             2 /* L2 pipeline has 2 stages */
#define N_L3_STAGES             2 /* L2 pipeline has 2 stages */
#define N_STAGES                N_L3_STAGES /* max stage size */
#define N_LAST_STAGE_BBQ        8

#define BBQ_QLEN                (1024 * 1024 * 32)
#define BBQ_TIMEOUT             (100 * 1000)
#define BBQ_BATCH_SIZE          (2048)


__thread uint32_t pipeline_worker_id;


static legacy_sw_stage_t s_stages[N_PIPELINES][N_STAGES];
static legacy_sw_stage_spec_t s_stage_specs[N_PIPELINES][N_STAGES];
static lagopus_bbq_t s_egress_bbq[N_PIPELINES][N_LAST_STAGE_BBQ];


/*
 * Definition of legacy switch pipelines.
 */
static struct pipeline_layout s_layouts[N_PIPELINES] = {
  {
    .name = "legacy_l2_sw",
    .n_stages = 2,
    .stages = {
      { mactable_port_learning, 1, 0 },
      { mactable_port_lookup, 1, 1 }
    }
  },
  {
    .name = "legacy_l3_sw",
    .n_stages = 2,
    .stages = {
      { mactable_port_learning, 1, 2 },
      { rib_lookup, 1, 3 }
    }
  }
};





static inline void
s_pipeline_destroy(legacy_sw_stage_t *stages,
                   size_t max_stage) {
  if (likely(stages != NULL)) {
    size_t i;

    for (i = 0; i < max_stage; i++) {
      lagopus_pipeline_stage_destroy((lagopus_pipeline_stage_t *)&stages[i]);
      stages[i] = NULL;
    }
  }
}


static inline lagopus_result_t
s_pipeline_create(legacy_sw_stage_t *stages,
                  const char *name,
                  size_t max_stage,
                  legacy_sw_stage_spec_t *stage_info) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  if (unlikely(stages == NULL || max_stage < 1 || stage_info == NULL))
    return LAGOPUS_RESULT_INVALID_ARGS;


  (void)memset((void *)stages, 0, sizeof(*stages) * max_stage);

  for (i = 0; i < max_stage; i++) {
    ret = s_legacy_sw_stage_create_by_spec(&(stages[i]),
                                           name,
                                           i,
                                           max_stage,
                                           &stage_info[i]);

    if (ret != LAGOPUS_RESULT_OK)
      goto done;
  }

done:
  if (unlikely(ret != LAGOPUS_RESULT_OK)) {
    s_pipeline_destroy(stages, i);
  }

  return ret;
}





static inline lagopus_result_t
s_init(enum pipeline_index idx) {
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  int i;

  for (i = 0; i < N_LAST_STAGE_BBQ; i++) {
    ret = lagopus_bbq_create(&s_egress_bbq[idx][i], uint64_t, BBQ_QLEN, NULL);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_exit_fatal("Cannot create bbq\n");
    }
  }
  return ret;
}


static inline void
s_final(__UNUSED enum pipeline_index idx) {
  return;
}





static lagopus_result_t
s_stage_main(const lagopus_pipeline_stage_t *sptr,
             size_t idx,
             void *evbuf,
             size_t n_evs) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct lagopus_packet **pkts;
  size_t stage_idx, n_stages, i;
  enum pipeline_index pipeline_idx;

  ret = s_base_get_stage_idx(sptr, &stage_idx, &n_stages);
  if (unlikely(ret != LAGOPUS_RESULT_OK)) {
    return ret;
  }

  pkts = (struct lagopus_packet **)evbuf;
  pipeline_idx = pkts[0]->pipeline_context.pipeline_idx;

  if (unlikely(pipeline_worker_id == 0)) {
    /* worker id starts at 1 */
    pipeline_worker_id = 1 + (uint32_t)idx +
            s_layouts[pipeline_idx].stages[stage_idx].worker_id_offset;
  }

  for (i = 0; i < n_evs; i++) {
    if (unlikely(pkts[i]->pipeline_context.error == true)) {
      continue;
    }

    if (likely(stage_idx < s_layouts[pipeline_idx].n_stages)) {
      s_layouts[pipeline_idx].stages[stage_idx].main_proc(pkts[i]);
    }
  }

  /* In the case of last stage, submit evbuf to egress bbq. */
  if (stage_idx == n_stages - 1) {
    static __thread int pos = 0;

    if (N_LAST_STAGE_BBQ > 1) {
      s_base_put_n(&s_egress_bbq[pipeline_idx][pos++ % N_LAST_STAGE_BBQ], evbuf, n_evs);
    } else {
      s_base_put_n(&s_egress_bbq[pipeline_idx][0], evbuf, n_evs);
    }
  }

#ifdef PERFORMANCE_TEST
  if (stage_idx == n_stages - 1) {
    static __thread uint64_t tsend;
    static __thread lagopus_chrono_t last_time, new_time;

    tsend += n_evs;
    WHAT_TIME_IS_IT_NOW_IN_NSEC(new_time);
    if (new_time - last_time > 3000000000) {
      printf("Last Stage PPS: %ld\n", tsend / 3);
      last_time = new_time;
      tsend = 0;
    }
  }
#endif

  return (lagopus_result_t)n_evs;
}





static inline lagopus_result_t
s_parse_args(enum pipeline_index idx) {
  size_t i;

  for (i = 0; i < s_layouts[idx].n_stages; i++) {
    s_stage_specs[idx][i].m_to = BBQ_TIMEOUT;
    s_stage_specs[idx][i].main_proc = s_stage_main;
    s_stage_specs[idx][i].m_n_workers = s_layouts[idx].stages[i].n_workers;

    s_stage_specs[idx][i].m_q_len = BBQ_QLEN;
    s_stage_specs[idx][i].m_batch_size = BBQ_BATCH_SIZE;
    s_stage_specs[idx][i].m_n_qs = 1;

    s_stage_specs[idx][i].m_sched_type = base_stage_sched_single;
    s_stage_specs[idx][i].m_fetch_type = base_stage_fetch_single;
  }

  return LAGOPUS_RESULT_OK;
}


static inline lagopus_result_t
s_create(enum pipeline_index idx) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = s_pipeline_create(s_stages[idx], s_layouts[idx].name,
                s_layouts[idx].n_stages, s_stage_specs[idx]);

  return ret;
}

static inline lagopus_result_t
s_run(enum pipeline_index idx) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_pipeline_stage_t *s;
  size_t i;
  int j;

  for (i = 0; i < s_layouts[idx].n_stages; i++) {
    s = (lagopus_pipeline_stage_t *)(&s_stages[idx][i]);
    ret = lagopus_pipeline_stage_setup(s);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }

    /* clear and set affinity */
    lagopus_pipeline_stage_set_worker_cpu_affinity(s, i, -1);
    for (j = 2; j < 64; j++) {
      ret = lagopus_pipeline_stage_set_worker_cpu_affinity(s, i, j);
      if (ret != LAGOPUS_RESULT_OK) {
        break;
      }
    }
  }

  /*
   * Start all the stages.
   */
  for (i = 0; i < s_layouts[idx].n_stages; i++) {
    s = (lagopus_pipeline_stage_t *)(&s_stages[idx][i]);
    ret = lagopus_pipeline_stage_start(s);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
  }

out:
  return ret;
}


static inline void
s_destroy(enum pipeline_index idx) {
  s_pipeline_destroy(s_stages[idx], s_layouts[idx].n_stages);
}





static inline lagopus_result_t
s_start(enum pipeline_index idx) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = s_create(idx);
  if (ret != LAGOPUS_RESULT_OK)
    goto out;

  ret = s_run(idx);
  if (ret != LAGOPUS_RESULT_OK)
    s_destroy(idx);

out:
  return ret;
}


static inline lagopus_result_t
s_shutdown(enum pipeline_index idx) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  size_t i;
  lagopus_pipeline_stage_t *s;

  for (i = 0; i < s_layouts[idx].n_stages; i++) {
    s = (lagopus_pipeline_stage_t *)(&s_stages[idx][i]);
    ret = lagopus_pipeline_stage_cancel(s);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
  }

  /*
   * Wake all the stages up, then all the stage workers exit.
   */
  for (i = 0; i < s_layouts[idx].n_stages; i++) {
    ret = s_legacy_sw_stage_wakeup(s_stages[idx][i]);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
  }

  /*
   * Wait all the stages finish.
   */
  for (i = 0; i < s_layouts[idx].n_stages; i++) {
    s = (lagopus_pipeline_stage_t *)(&s_stages[idx][i]);
    ret = lagopus_pipeline_stage_wait(s, -1LL);
    if (ret != LAGOPUS_RESULT_OK) {
      break;
    }
  }

out:
  return ret;
}


static inline lagopus_result_t
s_stop(__UNUSED enum pipeline_index idx) {

  return LAGOPUS_RESULT_OK;
}





lagopus_result_t
dp_legacy_sw_thread_init(__UNUSED int argc,
                         __UNUSED const char *const argv[],
                         __UNUSED void *extarg,
                         __UNUSED lagopus_thread_t **thdptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum pipeline_index idx;

  for (idx = 0; idx < MAX_PIPELINE; idx++) {
    ret = s_parse_args(idx);
    if (ret != LAGOPUS_RESULT_OK) {
      return ret;
    }
    ret = s_init(idx);
    if (ret != LAGOPUS_RESULT_OK) {
      return ret;
    }
  }
  return ret;
}


lagopus_result_t
dp_legacy_sw_thread_start(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum pipeline_index idx;

  for (idx = 0; idx < MAX_PIPELINE; idx++) {
    ret = s_start(idx);
    if (ret != LAGOPUS_RESULT_OK)
      return ret;
  }

  return ret;
}


void
dp_legacy_sw_thread_fini(void) {
  enum pipeline_index idx;

  for (idx = 0; idx < MAX_PIPELINE; idx++) {
    s_final(idx);
  }
}


lagopus_result_t
dp_legacy_sw_thread_shutdown(__UNUSED shutdown_grace_level_t level) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum pipeline_index idx;

  for (idx = 0; idx < MAX_PIPELINE; idx++) {
    ret = s_shutdown(idx);
    if (ret != LAGOPUS_RESULT_OK)
      return ret;
  }

  return ret;
}


lagopus_result_t
dp_legacy_sw_thread_stop(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum pipeline_index idx;

  for (idx = 0; idx < MAX_PIPELINE; idx++) {
    ret = s_stop(idx);
    if (ret != LAGOPUS_RESULT_OK)
      return ret;
  }

  return ret;
}





static inline lagopus_result_t
pipeline_put(enum pipeline_index idx, void *evbuf, size_t n_evs) {
  lagopus_pipeline_stage_t *sptr;

  sptr = (lagopus_pipeline_stage_t *)&s_stages[idx][0];
  return s_base_sched_single(sptr, evbuf, n_evs, NULL);
}


static inline lagopus_result_t
pipeline_get(enum pipeline_index idx, void *evbuf, size_t n_evs) {
  static __thread int pos = 0;

  if (N_LAST_STAGE_BBQ > 1) {
    return s_base_get_n(&s_egress_bbq[idx][pos++ % N_LAST_STAGE_BBQ],
                        evbuf, n_evs, BBQ_TIMEOUT);
  }
  return s_base_get_n(&s_egress_bbq[idx][0], evbuf, n_evs, BBQ_TIMEOUT);
}


static inline void
pipeline_forward(struct lagopus_packet **pkts, int n_pkts)
{
  int i;

  for (i = 0; i < n_pkts; i++) {
#ifndef PERFORMANCE_TEST
    if (pkts[i]->pipeline_context.error != true) {
      if (unlikely(pkts[i]->send_kernel)) {
        dp_interface_send_packet_kernel(pkts[i], pkts[i]->ifp);
      } else {
        lagopus_forward_packet_to_port_hybrid(pkts[i]);
      }
    } else {
      lagopus_packet_free(pkts[i]);
    }
#endif
  }
  return;
}


void
pipeline_process(struct lagopus_packet *pkt) {

  int n_pkts;
  size_t i;
  enum pipeline_index idx;
  static __thread size_t pos[N_PIPELINES];
  static __thread struct lagopus_packet *pkts[N_PIPELINES][BBQ_BATCH_SIZE];

  /* distinguish pipeline type, and store it */
  idx = pkt->pipeline_context.pipeline_idx;
  pkts[idx][pos[idx]] = pkt;
  pos[idx]++;

#ifdef PERFORMANCE_TEST
  lagopus_packet_free(pkt);
  if (pos[idx] == BBQ_BATCH_SIZE) {

    for (i = 0; i < 8; i++) {
      n_pkts = (int)pipeline_put(idx, pkts[idx], pos[idx]);
    }
#else
  if ((pkt->pipeline_context.is_last_packet_of_bulk == true) ||
      (pos[idx] == BBQ_BATCH_SIZE)) {
    n_pkts = (int)pipeline_put(idx, pkts[idx], pos[idx]);
#endif

    /* if error, free all packets */
    if (n_pkts < 0) {
      for (i = 0; i < pos[idx]; i++) {
        lagopus_packet_free(pkts[idx][i]);
      }
      pos[idx] = 0;
      return;
    }

    for (i = 0; i < N_LAST_STAGE_BBQ; i++) {
      n_pkts = (int)pipeline_get(idx, pkts[idx], BBQ_BATCH_SIZE);
      if (likely(n_pkts >= 0)) {
        pipeline_forward(pkts[idx], n_pkts);
      }
    }

    /* reset position */
    pos[idx] = 0;
  }
  return;
}

void
pipeline_process_stacked_packets(void) {
  static __thread struct lagopus_packet *pkts[BBQ_BATCH_SIZE];
  enum pipeline_index idx;
  int n_pkts;

  for (idx = 0; idx < MAX_PIPELINE; idx++) {
    /* just receive packets, then forward them */
    n_pkts = (int)pipeline_get(idx, pkts, BBQ_BATCH_SIZE);
    if (likely(n_pkts >= 0)) {
      pipeline_forward(pkts, n_pkts);
    }
  }
}

#endif /* HYBRID */
