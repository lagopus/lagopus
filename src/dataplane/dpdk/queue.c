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

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "lagopus_apis.h"

#include "lagopus/interface.h"

#include "rte_config.h"
#include "rte_sched.h"

lagopus_result_t
dpdk_interface_queue_add(struct interface *ifp, dp_queue_info_t *queue) {
  struct dp_ifqueue new_ifqueue;
  struct rte_meter_trtcm_params param;
  struct rte_meter_trtcm *meter;

  lagopus_result_t rv;
  int rte_err;

  if (ifp->ifqueue.nqueue >= DP_MAX_QUEUES) {
    return LAGOPUS_RESULT_TOO_MANY_OBJECTS;
  }
  if (ifp->ifqueue.nqueue >= RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE) {
    return LAGOPUS_RESULT_TOO_MANY_OBJECTS;
  }
  for (int i = 0; i < ifp->ifqueue.nqueue; i++) {
    if (ifp->ifqueue.queues[i]->id == queue->id) {
      return LAGOPUS_RESULT_ALREADY_EXISTS;
    }
  }

  memcpy(&new_ifqueue, &ifp->ifqueue, sizeof(new_ifqueue));
  memset(&new_ifqueue.stats[new_ifqueue.nqueue], 0, sizeof(new_ifqueue.stats[0]));

  param.cir = queue->committed_information_rate;
  param.pir = queue->peak_information_rate;
  param.cbs = queue->committed_burst_size;
  param.pbs = queue->peak_burst_size;

  meter = &new_ifqueue.meters[new_ifqueue.nqueue];
  if ((rte_err = rte_meter_trtcm_config(meter, &param)) != 0) {
    lagopus_msg_error("rte_meter_trtcm_config got error: %d\n", rte_err);
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  new_ifqueue.queues[new_ifqueue.nqueue] = queue;
  new_ifqueue.nqueue++;

  rv = dpdk_queue_configure(ifp, &new_ifqueue);
  if (rv != LAGOPUS_RESULT_OK) return rv;

  memcpy(&ifp->ifqueue, &new_ifqueue, sizeof(new_ifqueue));
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_interface_queue_delete(struct interface *ifp, uint32_t queue_id) {
  for (int i = 0; i < ifp->ifqueue.nqueue; i++) {
    if (ifp->ifqueue.queues[i]->id == queue_id) {
      ifp->ifqueue.nqueue--;
      memcpy(&ifp->ifqueue.queues[i], &ifp->ifqueue.queues[i + 1],
             ifp->ifqueue.nqueue - i);
      dpdk_queue_configure(ifp, &ifp->ifqueue);
      return LAGOPUS_RESULT_OK;
    }
  }
  return LAGOPUS_RESULT_NOT_FOUND;
}

// sort queues by their priorities in ascending order
static void sort_by_ascending_priority(struct dp_ifqueue *ifqueue) {
  int nqueue = ifqueue->nqueue;

  struct rte_sched_queue_stats stat;
  struct rte_meter_trtcm meter;
  dp_queue_info_t *queue;

  for(int i = 0; i < nqueue - 1; ++i) {
    for(int j = i + 1; j < nqueue; ++j) {
      if (ifqueue->queues[i]->priority > ifqueue->queues[j]->priority) {

        memcpy(&queue, &ifqueue->queues[i], sizeof(queue));
        memcpy(&ifqueue->queues[i], &ifqueue->queues[j], sizeof(queue));
        memcpy(&ifqueue->queues[j], &queue, sizeof(queue));

        memcpy(&meter, &ifqueue->meters[i], sizeof(meter));
        memcpy(&ifqueue->meters[i], &ifqueue->meters[j], sizeof(meter));
        memcpy(&ifqueue->meters[j], &meter, sizeof(meter));

        memcpy(&stat, &ifqueue->stats[i], sizeof(stat));
        memcpy(&ifqueue->stats[i], &ifqueue->stats[j], sizeof(stat));
        memcpy(&ifqueue->stats[j], &stat, sizeof(stat));
      }
    }
  }
}

lagopus_result_t
dpdk_queue_configure(struct interface *ifp, struct dp_ifqueue *ifqueue) {
  struct rte_sched_port_params params;
  struct rte_sched_subport_params subport_params;
  struct rte_sched_pipe_params pipe_params;
  struct rte_sched_port *new_sched;

  uint32_t rate_sum, burst_sum;
  unsigned lcore;
  int rte_err, i;

  if (ifqueue->nqueue == 0) {
    dpdk_queue_unconfigure(ifp);
    return LAGOPUS_RESULT_OK;
  }

  params.name = ifp->name;
  params.pipe_profiles = &pipe_params;
  app_get_lcore_for_nic_tx(ifp->info.eth_dpdk_phy.port_number, &lcore);
  params.socket = rte_lcore_to_socket_id(lcore);
  params.mtu = ifp->info.eth_dpdk_phy.mtu;
  params.frame_overhead = 12; /* minimum */

  params.n_subports_per_port = 1;
  params.n_pipes_per_subport = 1;
  params.n_pipe_profiles = 1;

  // set the same size for all queues
  for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
    params.qsize[i] = 128;
  }

  // assotiate each queue with its own traffic class
  // order queues in accordance to their priorities
  // the lower priority, the more packets processed
  sort_by_ascending_priority(ifqueue);

  // set up phony weights for queues within each traffic class
  for (i = 0; i < RTE_SCHED_QUEUES_PER_PIPE; i++) {
    pipe_params.wrr_weights[i] = 1; // must be positive
  }

  // enforcing upper limits for each traffic class queues
  //  bursts are used to set up the overall port policing
  burst_sum = rate_sum = 0;
  for (i = 0; i < ifqueue->nqueue; i++) {
    uint32_t burst, rate;
    if (ifqueue->queues[i]->peak_information_rate <= UINT32_MAX) {
      rate = (uint32_t)ifqueue->queues[i]->peak_information_rate;
    } else return LAGOPUS_RESULT_OUT_OF_RANGE;

    if (rate_sum > UINT32_MAX - rate) {
      rate_sum = UINT32_MAX;
    } else rate_sum += rate;

    subport_params.tc_rate[i] = rate;
    pipe_params.tc_rate[i] = rate;

    if (ifqueue->queues[i]->committed_burst_size <= UINT32_MAX) {
      burst = (uint32_t)ifqueue->queues[i]->committed_burst_size;
    } else return LAGOPUS_RESULT_OUT_OF_RANGE;

    if (burst_sum > UINT32_MAX - burst) {
      burst_sum = UINT32_MAX;
    } else burst_sum += burst;
  }

  for (; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
    subport_params.tc_rate[i] = 1; // must be positive
    pipe_params.tc_rate[i] = 1; // must be positive
  }

  // adjusting parameters of the token buckets
  //  to pass the traffic at all levels of the scheduler
  params.rate = rate_sum;

  subport_params.tc_period = 100;
  subport_params.tb_rate = rate_sum;
  subport_params.tb_size = burst_sum;

  pipe_params.tc_period = 100;
  pipe_params.tb_rate = rate_sum;
  pipe_params.tb_size = burst_sum;

#ifdef RTE_SCHED_RED
  for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
    int j;
    for (j = 0; j < e_RTE_METER_COLORS; j++) {
      params.red_params[i][j].min_th = 0;
      params.red_params[i][j].max_th = RTE_RED_MAX_TH_MAX;
      params.red_params[i][j].maxp_inv = RTE_RED_MAXP_INV_MAX;
      params.red_params[i][j].wq_log2 = RTE_RED_WQ_LOG2_MAX;
    }
  }
#endif /* RTE_SCHED_RED */

  // resetting the port scheduler
  if ((new_sched = rte_sched_port_config(&params)) == NULL) {
    lagopus_msg_error("rte_sched_port_config got error");
  } else
  if ((rte_err = rte_sched_subport_config(new_sched, 0, &subport_params)) != 0) {
    lagopus_msg_error("rte_sched_subport_config got error: %d\n", rte_err);
  } else
  if ((rte_err = rte_sched_pipe_config(new_sched, 0, 0, 0)) != 0) {
    lagopus_msg_error("rte_sched_pipe_config got error: %d\n", rte_err);
  } else {
    // everything worked as expected
    //  swapping the old scheduler with the new one
    if (!ifp->sched_port) rte_sched_port_free(ifp->sched_port);
    ifp->sched_port = new_sched;
    return LAGOPUS_RESULT_OK;
  }

  // scheduler initialization failed
  if (!new_sched) rte_sched_port_free(new_sched);
  return LAGOPUS_RESULT_ANY_FAILURES;
}


void
dpdk_queue_unconfigure(struct interface *ifp) {
  struct rte_sched_port *old_sched;

  ifp->ifqueue.nqueue = 0;
  old_sched = ifp->sched_port;
  ifp->sched_port = NULL;
  if (old_sched != NULL) {
    rte_sched_port_free(old_sched);
  }
}
