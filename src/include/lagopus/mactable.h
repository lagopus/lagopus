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

/**
 * Address type.
 */
enum address_type {
  MACTABLE_SETTYPE_STATIC = 0,
  MACTABLE_SETTYPE_DYNAMIC
};

/**
 * MAC address entry.
 */
struct macentry {
  TAILQ_ENTRY(macentry) next;
  uint64_t inteth; /**< Ethernet address.*/
  uint32_t portid; /**< Port number(ofp port no). */
  struct timespec update_time; /**< Referring to the time this entry to the last. */
  uint16_t address_type; /**< Setting address type. */
};

/**
 * MAC address table.
 */
struct mactable {
  lagopus_hashmap_t hashmap; /**< Hashmap for MAC address table. */
  unsigned int nentries; /**< Current number of entries in this table. */
  uint32_t maxentries;   /**< Max number of entries for this table. */
  lagopus_rwlock_t lock; /**< Read-write lock. */
  TAILQ_HEAD(macentry_list, macentry) macentry_list; /**< MAC address entry list. */

  uint32_t ageing_time;  /**< Aging time(default 300sec). */
};

/**
 * Initialize the mactable.
 * @param[in] mactable MAC address table.
 * @retval    LAGOPUS_RESULT_OK            Succeeded.
 * @retval    LAGOPUS_RESULT_INVALID_ARGS  Arguments are invalid.
 * @retval    LAGOPUS_RESULT_NO_MEMORY     Memory exhausted.
 */
lagopus_result_t
init_mactable(struct mactable *mactable);

/**
 * Finalize the mactable.
 * @param[in] mactable MAC address table.
 * @retval    LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
fini_mactable(struct mactable *mactable);

/**
 * Find port and learn mac address in mactable.
 * @param[in] pkt Packet data.
 * @retval    !=OFPP_ALL  Output port number.
 * @retval    ==OFPP_ALL  No corresponding data, packet will be flooding.
 */
uint32_t
find_and_learn_port_in_mac_table(struct lagopus_packet *pkt);

/**
 * Set ageing time.
 * @param[in] mactable MAC address table.
 * @param[in] ageing_time Ageing time of mac address table.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 */
void
mactable_set_ageing_time(struct mactable *mactable, uint32_t ageing_time);

/**
 * Set num of max entries.
 * @param[in] mactable MAC address table.
 * @param[in] max_entries Number of max entries of mac address table.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 * @retval    LAGOPUS_RESULT_INVALID_ARGS     Arguments are invalid.
 * @retval    LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
void
mactable_set_max_entries(struct mactable *mactable, uint32_t max_entries);

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
 * Delete a entry.
 * @param[in] mactable MAC address table.
 * @param[in] ethaddr MAC address.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
mactable_entry_delete(struct mactable *mactable, const uint8_t ethaddr[]);

/**
 * Clear all entries in mactable.
 * @param[in] mactable MAC address table.
 * @retval    LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
mactable_entry_clear(struct mactable *mactable);

/**
 * Get ageing time.
 * @param[in] mactable MAC address table.
 * @retval    Ageing time.
 */
uint32_t
mactable_get_ageing_time(struct mactable *mactable);

/**
 * Get number of max entries.
 * @param[in] mactable MAC address table.
 * @retval    Number of max entries..
 */
uint32_t
mactable_get_max_entries(struct mactable *mactable);

#endif /* SRC_INCLUDE_LAGOPUS_MACTABLE_H_ */
