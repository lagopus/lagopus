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
 *      @brief  OpenFlow Meter management.
 */

#include "lagopus_apis.h"
#include "lagopus/meter.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"

#include "lock.h"

/**
 * @brief Meter table.
 */
struct meter_table {                    /** Meter table. */
  pthread_rwlock_t rwlock;              /** Read-write lock. */
  lagopus_hashmap_t hashmap;            /** Meter id hashtable. */
};

static inline void
meter_table_lock_init(struct meter_table *meter_table) {
  (void) meter_table;
}

static inline void
meter_table_rdlock(struct meter_table *meter_table) {
  (void) meter_table;

  FLOWDB_RWLOCK_RDLOCK();
}

static inline void
meter_table_rdunlock(struct meter_table *meter_table) {
  (void) meter_table;

  FLOWDB_RWLOCK_RDUNLOCK();
}

static inline void
meter_table_wrlock(struct meter_table *meter_table) {
  (void) meter_table;

  FLOWDB_UPDATE_BEGIN();
}

static inline void
meter_table_wrunlock(struct meter_table *meter_table) {
  (void) meter_table;

  FLOWDB_UPDATE_END();
}

static struct meter *
meter_alloc(uint32_t meter_id,
            uint16_t flags,
            struct meter_band_list *band_list) {
  struct meter *meter;

  meter = calloc(1, sizeof(struct meter));
  if (meter == NULL) {
    return NULL;
  }

  meter->meter_id = meter_id;
  meter->flags = flags;
  TAILQ_INIT(&meter->band_list);
  TAILQ_CONCAT(&meter->band_list, band_list, entry);
  clock_gettime(CLOCK_MONOTONIC, &meter->create_time);
  if (lagopus_register_meter != NULL) {
    lagopus_register_meter(meter);
  }

  return meter;
}

static lagopus_result_t
meter_modify(struct meter *meter,
             uint16_t flags,
             struct meter_band_list *band_list) {
  struct meter_band *band;

  while ((band = TAILQ_FIRST(&meter->band_list)) != NULL) {
    TAILQ_REMOVE(&meter->band_list, band, entry);
    meter_band_free(band);
  }
  if (lagopus_unregister_meter != NULL) {
    lagopus_unregister_meter(meter);
  }
  meter->flags = flags;
  TAILQ_INIT(&meter->band_list);
  TAILQ_CONCAT(&meter->band_list, band_list, entry);
  if (lagopus_register_meter != NULL) {
    lagopus_register_meter(meter);
  }

  return LAGOPUS_RESULT_OK;
}

static void
meter_free(struct meter *meter) {
  struct meter_band *band;

  while ((band = TAILQ_FIRST(&meter->band_list)) != NULL) {
    TAILQ_REMOVE(&meter->band_list, band, entry);
    meter_band_free(band);
  }

  if (lagopus_unregister_meter != NULL) {
    lagopus_unregister_meter(meter);
  }
  free(meter);
}

struct meter_table *
meter_table_alloc(void) {
  struct meter_table *meter_table;

  meter_table = calloc(1, sizeof(struct meter_table));
  if (meter_table == NULL) {
    return NULL;
  }

  /* 32bit meter_id table. */
  if (lagopus_hashmap_create(&meter_table->hashmap,
                             LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                             meter_free) != LAGOPUS_RESULT_OK) {
    free(meter_table);
    return NULL;
  }

  meter_table_lock_init(meter_table);

  return meter_table;
}

void
meter_table_free(struct meter_table *meter_table) {
  lagopus_hashmap_destroy(&meter_table->hashmap, true);
  free(meter_table);
}

lagopus_result_t
meter_table_meter_add(struct meter_table *meter_table,
                      struct ofp_meter_mod *mod,
                      struct meter_band_list *band_list,
                      struct ofp_error *error) {
  uint32_t key;
  struct meter *meter;
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  (void)error;

  /* Write lock the meter_table. */
  meter_table_wrlock(meter_table);

  /* Key is network byte order. */
  key = htonl(mod->meter_id);

  /* Lookup node. */
  rv = lagopus_hashmap_find_no_lock(&meter_table->hashmap,
                                    (void *)key, &meter);
  if (rv == LAGOPUS_RESULT_OK) {
    error->type = OFPET_METER_MOD_FAILED;
    error->code = OFPMMFC_METER_EXISTS;
    lagopus_msg_info("meter add: %d: meter is exist (%d:%d)",
                     mod->meter_id, error->type, error->code);
    rv = LAGOPUS_RESULT_OFP_ERROR;
    goto out;
  }

  meter = meter_alloc(mod->meter_id, mod->flags, band_list);
  if (meter == NULL) {
    rv = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }
  rv = lagopus_hashmap_add_no_lock(&meter_table->hashmap,
                                   (void *)key, &meter, false);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

out:
  ofp_meter_band_list_elem_free(band_list);

  /* Unlock the meter_table then return result. */
  meter_table_wrunlock(meter_table);
  return rv;
}

lagopus_result_t
meter_table_meter_modify(struct meter_table *meter_table,
                         struct ofp_meter_mod *mod,
                         struct meter_band_list *band_list,
                         struct ofp_error *error) {
  uint32_t key;
  struct meter *meter;
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  (void)error;

  /* Write lock the meter_table. */
  meter_table_wrlock(meter_table);

  /* Key is network byte order. */
  key = htonl(mod->meter_id);

  /* Lookup node. */
  ret = lagopus_hashmap_find_no_lock(&meter_table->hashmap,
                                     (void *)key, &meter);
  if (ret != LAGOPUS_RESULT_OK) {
    error->type = OFPET_METER_MOD_FAILED;
    error->code = OFPMMFC_UNKNOWN_METER;
    lagopus_msg_info("meter modify: %d: meter is not exist (%d:%d)",
                     mod->meter_id, error->type, error->code);
    ret = LAGOPUS_RESULT_OFP_ERROR;
    goto out;
  }

  /* Modify meter with new parameter. */
  ret = meter_modify(meter, mod->flags, band_list);
  if (ret != LAGOPUS_RESULT_OK) {
    goto out;
  }

out:
  ofp_meter_band_list_elem_free(band_list);

  /* Unlock the meter_table then return result. */
  meter_table_wrunlock(meter_table);
  return ret;
}

lagopus_result_t
meter_table_meter_delete(struct meter_table *meter_table,
                         struct ofp_meter_mod *mod,
                         struct ofp_error *error) {
  uint32_t key;
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  (void)error;

  /* Write lock the meter_table. */
  meter_table_wrlock(meter_table);

  if (mod->meter_id == OFPM_ALL) {
    lagopus_hashmap_clear_no_lock(&meter_table->hashmap, true);
  } else {

    /* Key is network byte order. */
    key = htonl(mod->meter_id);

    /* Lookup node. */
    ret = lagopus_hashmap_delete_no_lock(&meter_table->hashmap,
                                         (void *)key, NULL, true);
  }
  /* Unlock the meter_table then return result. */
  meter_table_wrunlock(meter_table);
  return ret;
}

struct meter *
meter_table_lookup(struct meter_table *meter_table, uint32_t meter_id) {
  uint32_t key = htonl(meter_id);
  struct meter *meter;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find_no_lock(&meter_table->hashmap,
                                    (void *)key, &meter);
  if (rv != LAGOPUS_RESULT_OK) {
    return NULL;
  }
  return meter;
}

union meter_band_union {
  struct ofp_meter_band_header header;
  struct ofp_meter_band_drop drop;
  struct ofp_meter_band_dscp_remark remark;
  struct ofp_meter_band_experimenter experimenter;
};

struct meter_band *
meter_band_alloc(struct ofp_meter_band_header *band_header) {
  struct meter_band *band;
  union meter_band_union *band_union;

  band = calloc(1, sizeof(struct meter_band));
  if (band == NULL) {
    return NULL;
  }

  band_union = (union meter_band_union *)band_header;

  switch (band_header->type) {
    case OFPMBT_DROP:
      band->type = band_union->drop.type;
      band->len = band_union->drop.len;
      band->rate = band_union->drop.rate;
      band->burst_size = band_union->drop.burst_size;
      break;
    case OFPMBT_DSCP_REMARK:
      band->type = band_union->remark.type;
      band->len = band_union->remark.len;
      band->rate = band_union->remark.rate;
      band->burst_size = band_union->remark.burst_size;
      band->prec_level = band_union->remark.prec_level;
      break;
    case OFPMBT_EXPERIMENTER:
      band->type = band_union->experimenter.type;
      band->len = band_union->experimenter.len;
      band->rate = band_union->experimenter.rate;
      band->burst_size = band_union->experimenter.burst_size;
      band->experimenter = band_union->experimenter.experimenter;
      break;
    default:
      meter_band_free(band);
      return NULL;
  }

  return band;
}

void
meter_band_free(struct meter_band *band) {
  free(band);
}

static lagopus_result_t
set_meter_stats(struct meter_stats *stats, const struct meter *meter) {

  struct timespec ts;
  struct meter_band *band;

  stats->ofp.meter_id = meter->meter_id;
  stats->ofp.flow_count = meter->flow_count;
  stats->ofp.packet_in_count = meter->input_packet_count;
  stats->ofp.byte_in_count = meter->input_byte_count;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  stats->ofp.duration_sec = (uint32_t)(ts.tv_sec - meter->create_time.tv_sec);
  if (ts.tv_nsec < meter->create_time.tv_nsec) {
    stats->ofp.duration_sec--;
    stats->ofp.duration_nsec = 1 * 1000 * 1000 * 1000;
  } else {
    stats->ofp.duration_nsec = 0;
  }
  stats->ofp.duration_nsec += (uint32_t)ts.tv_nsec;
  stats->ofp.duration_nsec -= (uint32_t)meter->create_time.tv_nsec;

  /* band stats */
  TAILQ_INIT(&stats->meter_band_stats_list);
  TAILQ_FOREACH(band, &meter->band_list, entry) {
    struct meter_band_stats *band_stats;

    band_stats = calloc(1, sizeof(struct meter_band_stats));
    if (band_stats == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    band_stats->ofp = band->stats;
    TAILQ_INSERT_TAIL(&stats->meter_band_stats_list, band_stats, entry);
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
meter_add_stats(struct meter *meter, struct meter_stats_list *list) {
  if (meter != NULL) {
    struct meter_stats *stats;

    stats = calloc(1, sizeof(struct meter_stats));
    if (stats == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    set_meter_stats(stats, meter);
    TAILQ_INSERT_TAIL(list, stats, entry);
  }
  return LAGOPUS_RESULT_OK;
}

static bool
meter_do_stats_iterate(void *key, void *val,
                       lagopus_hashentry_t he, void *arg) {
  struct meter_stats_list *list;

  (void) key;
  (void) val;
  (void) he;

  list = arg;
  if (meter_add_stats(val, list) == LAGOPUS_RESULT_OK) {
    return true;
  } else {
    return false;
  }
}

lagopus_result_t
get_meter_stats(struct meter_table *meter_table,
                struct ofp_meter_multipart_request *request,
                struct meter_stats_list *list,
                struct ofp_error *error) {
  (void)error;
  if (request->meter_id == OFPM_ALL) {
    lagopus_hashmap_iterate(&meter_table->hashmap,
                            meter_do_stats_iterate,
                            list);
  } else {
    struct meter *meter;

    meter = meter_table_lookup(meter_table, request->meter_id);
    return meter_add_stats(meter, list);
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
set_meter_config(struct meter_config *config, const struct meter *meter) {

  struct meter_band *band;

  config->ofp.meter_id = meter->meter_id;
  config->ofp.flags = meter->flags;

  /* band config */
  TAILQ_INIT(&config->band_list);
  TAILQ_FOREACH(band, &meter->band_list, entry) {
    struct meter_band *band_config;

    band_config = calloc(1, sizeof(struct meter_band));
    if (band_config == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    *band_config = *band;
    TAILQ_INSERT_TAIL(&config->band_list, band_config, entry);
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
meter_add_config(struct meter *meter, struct meter_config_list *list) {
  if (meter != NULL) {
    struct meter_config *config;

    config = calloc(1, sizeof(struct meter_config));
    if (config == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    set_meter_config(config, meter);
    TAILQ_INSERT_TAIL(list, config, entry);
  }
  return LAGOPUS_RESULT_OK;
}

static bool
meter_do_config_iterate(void *key, void *val,
                       lagopus_hashentry_t he, void *arg) {
  struct meter_config_list *list;

  (void) key;
  (void) val;
  (void) he;

  list = arg;
  if (meter_add_config(val, list) == LAGOPUS_RESULT_OK) {
    return true;
  } else {
    return false;
  }
}

lagopus_result_t
get_meter_config(struct meter_table *meter_table,
                 struct ofp_meter_multipart_request *request,
                 struct meter_config_list *list,
                 struct ofp_error *error) {
  (void)error;
  if (request->meter_id == OFPM_ALL) {
    lagopus_hashmap_iterate(&meter_table->hashmap,
                            meter_do_config_iterate,
                            list);
  } else {
    struct meter *meter;

    meter = meter_table_lookup(meter_table, request->meter_id);
    return meter_add_config(meter, list);
  }

  return LAGOPUS_RESULT_OK;
}

/* Free band list elements. */
void
ofp_meter_band_list_elem_free(struct meter_band_list *band_list) {
  struct meter_band *band;

  while (TAILQ_EMPTY(band_list) == false) {
    band = TAILQ_FIRST(band_list);
    TAILQ_REMOVE(band_list, band, entry);
    free(band);
  }
}

/* MeterMod */
lagopus_result_t
ofp_meter_mod_add(uint64_t dpid,
                  struct ofp_meter_mod *meter_mod,
                  struct meter_band_list *band_list,
                  struct ofp_error *error) {
  struct bridge *bridge;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  ret = meter_table_meter_add(bridge->meter_table, meter_mod,
                              band_list, error);

  return ret;
}

lagopus_result_t
ofp_meter_mod_modify(uint64_t dpid,
                     struct ofp_meter_mod *meter_mod,
                     struct meter_band_list *band_list,
                     struct ofp_error *error) {
  struct bridge *bridge;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  ret = meter_table_meter_modify(bridge->meter_table, meter_mod,
                                 band_list, error);
  return ret;
}

lagopus_result_t
ofp_meter_mod_delete(uint64_t dpid,
                     struct ofp_meter_mod *meter_mod,
                     struct ofp_error *error) {
  struct bridge *bridge;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  ret = meter_table_meter_delete(bridge->meter_table, meter_mod,
                                 error);

  return ret;
}


lagopus_result_t
get_meter_features(struct meter_table *meter_table,
                   struct ofp_meter_features *features,
                   struct ofp_error *error) {
  (void)meter_table;
  (void)error;

  features->max_meter = OFPM_MAX;
  features->band_types =  (1 << OFPMBT_DROP) | (1 << OFPMBT_DSCP_REMARK);
  features->capabilities = OFPMF_KBPS | OFPMF_PKTPS | OFPMF_STATS;
  features->max_bands = LAGOPUS_METER_MAX_BANDS;
  features->max_color = 1;

  return LAGOPUS_RESULT_OK;
}

/*
 * meter_stats (Agent/DP API)
 */
lagopus_result_t
ofp_meter_stats_get(uint64_t dpid,
                    struct ofp_meter_multipart_request *meter_request,
                    struct meter_stats_list *meter_stats_list,
                    struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return get_meter_stats(bridge->meter_table,
                         meter_request,
                         meter_stats_list,
                         error);
}

/*
 * meter_config (Agent/DP API)
 */
lagopus_result_t
ofp_meter_config_get(uint64_t dpid,
                     struct ofp_meter_multipart_request *meter_request,
                     struct meter_config_list *meter_config_list,
                     struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return get_meter_config(bridge->meter_table,
                          meter_request,
                          meter_config_list,
                          error);
}

/*
 * meter_features (Agent/DP API)
 */
lagopus_result_t
ofp_meter_features_get(uint64_t dpid,
                       struct ofp_meter_features *meter_features,
                       struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return get_meter_features(bridge->meter_table,
                            meter_features,
                            error);
}
