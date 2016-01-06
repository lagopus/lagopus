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
  int i;

  if (ifp->ifqueue.nqueue >= DP_MAX_QUEUES) {
    return LAGOPUS_RESULT_TOO_MANY_OBJECTS;
  }
  for (i = 0; i < ifp->ifqueue.nqueue; i++) {
    if (ifp->ifqueue.queues[i]->id == queue->id) {
      return LAGOPUS_RESULT_ALREADY_EXISTS;
    }
  }
  memcpy(&new_ifqueue, &ifp->ifqueue, sizeof(new_ifqueue));
  new_ifqueue.queues[new_ifqueue.nqueue] = queue;
  param.cir = queue->committed_information_rate;
  param.pir = queue->peak_information_rate;
  param.cbs = queue->committed_burst_size;
  param.pbs = queue->peak_burst_size;
  rte_meter_trtcm_config(&new_ifqueue.meters[new_ifqueue.nqueue], &param);
  new_ifqueue.nqueue++;
  dpdk_queue_configure(ifp, &new_ifqueue);
  memcpy(&ifp->ifqueue, &new_ifqueue, sizeof(new_ifqueue));
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_interface_queue_delete(struct interface *ifp, uint32_t queue_id) {
  struct dp_ifqueue new_ifqueue;
  int i;

  for (i = 0; i < ifp->ifqueue.nqueue; i++) {
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

lagopus_result_t
dpdk_queue_configure(struct interface *ifp, struct dp_ifqueue *ifqueue) {
  struct rte_sched_port_params params;
  struct rte_sched_subport_params subport_params;
  struct rte_sched_pipe_params pipe_params;
  struct rte_sched_port *old_sched, *new_sched;
  unsigned lcore;
  int i;

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
  params.rate = 0;
  subport_params.tc_period = 100;
  subport_params.tb_rate = 0;
  subport_params.tb_size = 0;
  pipe_params.tb_rate = 0;
  pipe_params.tb_size = 0;
  for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
    params.qsize[i] = 128;
  }
  for (i = 0; i < ifqueue->nqueue; i++) {
    params.rate += ifqueue->queues[i]->peak_information_rate;
    subport_params.tb_rate += ifqueue->queues[i]->peak_information_rate;
    subport_params.tb_size += ifqueue->queues[i]->committed_burst_size;
    pipe_params.tb_rate += ifqueue->queues[i]->peak_information_rate;
    pipe_params.tb_size += ifqueue->queues[i]->committed_burst_size;
    pipe_params.wrr_weights[i] = ifqueue->queues[i]->priority;
  }
  for (; i < RTE_SCHED_QUEUES_PER_PIPE; i++) {
    pipe_params.wrr_weights[i] = 1;
  }
  for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
    subport_params.tc_rate[i] = subport_params.tb_rate;
    pipe_params.tc_rate[i] = pipe_params.tb_rate;
  }
  pipe_params.tc_period = 100;
  params.n_pipe_profiles = 1;

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

  new_sched = rte_sched_port_config(&params);
  if (new_sched == NULL) {
    lagopus_msg_error("rte_sched_port_config got error\n");
  }
  if (rte_sched_subport_config(new_sched, 0, &subport_params) != 0) {
    lagopus_msg_error("rte_sched_subport_config got error\n");
  }
  if (rte_sched_pipe_config(new_sched, 0, 0, 0) != 0) {
    lagopus_msg_error("rte_sched_pipe_config got error\n");
  }
  old_sched = ifp->sched_port;
  ifp->sched_port = new_sched;
  if (old_sched != NULL) {
    rte_sched_port_free(old_sched);
  }
  return LAGOPUS_RESULT_OK;
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
