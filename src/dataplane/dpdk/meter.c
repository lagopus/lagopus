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
 *      @file   meter.c
 *      @brief  Metering support with Intel DPDK
 */

#include <sys/queue.h>
#include <stdlib.h>

#include <rte_config.h>
#include <rte_cycles.h>
#include <rte_meter.h>

#include <openflow.h>
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"

#undef METER_DEBUG
#ifdef METER_DEBUG
#define DPRINT(...) printf(__VA_ARGS__)
#else
#define DPRINT(...)
#endif

#define KBPS2BYTEPS(kbps) ((kbps) * 1000 / 8)

/*
 * implementation of OpwnFlow meter and (rate) queue.
 * meter is ralated with flow entry,
 * queue is related with output port.
 *
 * DPDK rte_meter defines committed (minimum) rate and
 * peak (maxmimum) rate and burst size.
 */

struct lagopus_band {
  TAILQ_ENTRY(lagopus_band) next;
  struct meter_band *band;
  union {
    struct rte_meter_srtcm kbps_meter;
    struct rte_meter_srtcm pps_meter;
  };
};

TAILQ_HEAD(lagopus_band_list, lagopus_band);

static void dpdk_register_meter(struct meter *);
static void dpdk_unregister_meter(struct meter *);

void
lagopus_meter_init(void) {
  lagopus_register_meter = dpdk_register_meter;
  lagopus_unregister_meter = dpdk_unregister_meter;
}

/**
 * ofp_meter_band_flags has multiple type of meter.
 * - kbps
 * - pps
 * - burst size
 * - collect statistics
 *
 * srtcm parameters are:
 *  cir - commited information rate, in bytes per second
 *  cbs - commited burst size, in bytes
 *  ebs - excess burst size, in bytes
 */
static void
dpdk_register_meter(struct meter *meter) {
  struct meter_band *band;
  struct lagopus_band_list *list;
  struct rte_meter_srtcm_params param;

  DPRINT("registering meter, id=%d\n", meter->meter_id);
  list = calloc(1, sizeof(struct lagopus_band_list));
  if (list == NULL) {
    return;
  }
  TAILQ_INIT(list);
  TAILQ_FOREACH(band, &meter->band_list, entry) {
    struct lagopus_band *lband;

    lband = calloc(1, sizeof(struct lagopus_band));
    if (lband == NULL) {
      return;
    }
    lband->band = band;
    if ((meter->flags & OFPMF_PKTPS) == 0) {
      /* unit of rate is kbps, DPDK needs Bytes/sec */
      DPRINT("rate limit: %d kilo bit per second\n", band->rate);
      param.cir = KBPS2BYTEPS(band->rate);
      if ((meter->flags & OFPMF_BURST) != 0) {
        param.ebs = KBPS2BYTEPS(band->burst_size);
        param.cbs = KBPS2BYTEPS(band->rate);
      } else {
        param.ebs = 0;
        param.cbs = KBPS2BYTEPS(band->rate);
      }
      rte_meter_srtcm_config(&lband->kbps_meter, &param);
    } else {
      /* unit of rate is pps */
      DPRINT("rate limit: %d packet per second\n", band->rate);
      param.cir = band->rate;
      if ((meter->flags & OFPMF_BURST) != 0) {
        param.ebs = band->burst_size;
        param.cbs = band->rate;
      } else {
        param.ebs = 0;
        param.cbs = band->rate;
      }
      rte_meter_srtcm_config(&lband->pps_meter, &param);
    }
    TAILQ_INSERT_TAIL(list, lband, next);
  }
  meter->driverdata = list;
}

void
dpdk_unregister_meter(struct meter *meter) {
  free(meter->driverdata);
}

int
lagopus_meter_packet(struct lagopus_packet *pkt, struct meter *meter,
                     uint8_t *prec_level) {
  enum rte_meter_color color;
  struct lagopus_band_list *list;
  struct lagopus_band *lband, *color_band;

  DPRINT("metering packet\n");
  if ((meter->flags & OFPMF_STATS) != 0) {
    meter->input_packet_count++;
    meter->input_byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
  }
  list = meter->driverdata;
  color_band = NULL;
  if ((meter->flags & OFPMF_PKTPS) == 0) {
    TAILQ_FOREACH(lband, list, next) {
      color = rte_meter_srtcm_color_blind_check(&lband->kbps_meter,
                                                rte_rdtsc(),
                                                OS_M_PKTLEN(PKT2MBUF(pkt)));
      if (color_band == NULL && color == e_RTE_METER_RED) {
        color_band = lband;
      }
    }
  } else {
    TAILQ_FOREACH(lband, list, next) {
      color = rte_meter_srtcm_color_blind_check(&lband->pps_meter,
              rte_rdtsc(),
              1);
      if (color_band == NULL && color == e_RTE_METER_RED) {
        color_band = lband;
      }
    }
  }
  if (color_band != NULL) {
    DPRINT("color == red\n");
    if ((meter->flags & OFPMF_STATS) != 0) {
      color_band->band->stats.packet_band_count++;
      color_band->band->stats.byte_band_count += OS_M_PKTLEN(PKT2MBUF(pkt));
    }
    if (color_band->band->type == OFPMBT_DSCP_REMARK) {
      *prec_level = color_band->band->prec_level;
    }
    return color_band->band->type;
  }
  DPRINT("color != red\n");
  return 0;
}
