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
 * @file        mactable.h
 * @brief       MAC address table for realizing the learning bridge.
 */


#ifndef SRC_INCLUDE_LAGOPUS_MACTABLE_H_
#define SRC_INCLUDE_LAGOPUS_MACTABLE_H_

#include "updater.h"

/* ether addr history size */
#define MACTABLE_HISTORY_MAX_NUM (10)

/**
 * Address type.
 */
enum address_type {
  MACTABLE_SETTYPE_STATIC = 0,
  MACTABLE_SETTYPE_DYNAMIC
};

/**
 * Local data for each worker.
 */
struct local_data {
  lagopus_hashmap_t localcache;
  lagopus_bbq_t bbq;
  uint64_t eth_history[MACTABLE_HISTORY_MAX_NUM];
  uint16_t history_index;
  uint32_t referred_table;
  uint16_t referring;
} __attribute__ ((aligned(128)));

/**
 * MAC address entry.
 */
struct macentry {
  TAILQ_ENTRY(macentry) next;
  uint64_t inteth; /**< Ethernet address.*/
  uint32_t portid; /**< Port number(ofp port no). */
  struct timespec update_time; /**< Referring to the time this entry to the last. */
  uint16_t address_type; /**< Setting address type. */
  lagopus_rwlock_t lock; /**< Read-write lock for mactable entry. */
};

/**
 * MAC address table.
 */
struct mactable {
  lagopus_rwlock_t lock;        /**< Read-write lock. */

  uint32_t maxentries;          /**< Max number of entries for this table. */
  uint32_t ageing_time;         /**< Aging time(default 300sec). */
  unsigned int nentries;        /**< Current number of entries in this table. */

  lagopus_hashmap_t hashmap[2]; /**< Hashmap for MAC address table. */
  uint32_t read_table;          /**< Current read table index. */

  TAILQ_HEAD(macentry_list, macentry) macentry_list; /**< MAC address entry list. */

  struct local_data local[UPDATER_LOCALDATA_MAX_NUM];
};

/**
 * Initialize mactable object.
 * This function must be called when the bridge object creation.
 * @param[in] mactable MAC address table object.
 * @retval    LAGOPUS_RESULT_OK            Succeeded.
 * @retval    LAGOPUS_RESULT_INVALID_ARGS  Arguments are invalid.
 * @retval    LAGOPUS_RESULT_NO_MEMORY     Memory exhausted.
 */
lagopus_result_t
mactable_init(struct mactable *mactable);

/**
 * Finalize mactable object.
 * This function must be called when the bridge object discarded.
 * @param[in] mactable MAC address table.
 * @retval    LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
mactable_fini(struct mactable *mactable);

/**
 * Update mac address table by timer('updater').
 * Entry data are written to the writing mac table by only 'updater'.
 * They are merged read mac table and bbq.
 * @param[in] mactable MAC address table object.
 */
lagopus_result_t
mactable_update(struct mactable *mactable);

/**
 * Learning mac address and input port number when packet handling.
 * This function is called from l3 routing function in interface.c.
 * @param[in] pkt Packet data.
 */
void
mactable_port_learning(struct lagopus_packet *pkt);

/**
 * Look up output port in mac address table when packet handling.
 * This function is called from l3 routing function in interface.c.
 * @param[in] pkt Packet data.
 * @retval    !=OFPP_ALL  Output port number.
 * @retval    ==OFPP_ALL  No corresponding data, packet will be flooding.
 */
void
mactable_port_lookup(struct lagopus_packet *pkt);

/**
 * Clear all entries in mactable.
 * @param[in] mactable MAC address table.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
mactable_entry_clear(struct mactable *mactable);

/**
 * Delete a entry.
 * @param[in] mactable MAC address table.
 * @param[in] ethaddr MAC address.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
mactable_entry_delete(struct mactable *mactable, const uint8_t ethaddr[]);

/**
 * Update entry informations.
 * @param[in] mactable MAC address table.
 * @param[in] ethaddr MAC address.
 * @param[in] portid Port number that corresponding to MAC address.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
mactable_entry_update(struct mactable *mactable, const uint8_t ethaddr[], uint32_t portid);

/**
 * Get number of max entries.
 * @param[in] mactable MAC address table.
 * @retval    Number of max entries..
 */
uint32_t
mactable_max_entries_get(struct mactable *mactable);

/**
 * Get ageing time.
 * @param[in] mactable MAC address table.
 * @retval    Ageing time.
 */
uint32_t
mactable_ageing_time_get(struct mactable *mactable);

/**
 * Set num of max entries.
 * @param[in] mactable MAC address table.
 * @param[in] max_entries Number of max entries of mac address table.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 * @retval    LAGOPUS_RESULT_INVALID_ARGS     Arguments are invalid.
 * @retval    LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
void
mactable_max_entries_set(struct mactable *mactable, uint32_t max_entries);

/**
 * Set ageing time.
 * @param[in] mactable MAC address table.
 * @param[in] ageing_time Ageing time of mac address table.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 */
void
mactable_ageing_time_set(struct mactable *mactable, uint32_t ageing_time);

#endif /* SRC_INCLUDE_LAGOPUS_MACTABLE_H_ */
