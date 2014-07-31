/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#ifndef __CHANNEL_MGR_H__
#define __CHANNEL_MGR_H__

/**
 * @file        channel_mgr.h
 */

/**
 * Initialize a channel manager.
 *
 *  @param[in] arg  A event manager porinter.
 *
 *  @details Call this function for initializing a channel manager singleton object.
 *  pthread_once() is used in this function.
 */
void channel_mgr_initialize(void *arg);

/**
 * Finalize a channel manager.
 *
 *  @details Call this function for finishing a channel manager.
 */
void channel_mgr_finalize(void);

/**
 * Put a tcp channel created with a controller address into channel hash.
 *
 *  @param[in] bridge  A bridge pointer.
 *  @param[in] dpid    Datapath ID.
 *  @param[in] addr    A controller address.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 */
lagopus_result_t
channel_mgr_channel_add(struct bridge *bridge, uint64_t dpid,
                        struct addrunion *addr);


/**
 * Put a tls channel created with a controller address into channel hash.
 *
 *  @param[in] bridge  A bridge pointer.
 *  @param[in] dpid    Datapath ID.
 *  @param[in] addr    A controller address.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 */
lagopus_result_t
channel_mgr_channel_tls_add(struct bridge *bridge, uint64_t dpid,
                            struct addrunion *addr);


/**
 * Look up a channel that has this bridge and controller address.
 *
 *  @param[in] bridge  A bridge pointer.
 *  @param[in] addr    A controller address.
 *  @param[out] channel  A channel.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 */
lagopus_result_t
channel_mgr_channel_lookup(struct bridge *bridge, struct addrunion *addr,
                           struct channel **channel);


/**
 * Remove a channel that has this bridge and controller address from channel hash.
 *
 *  @param[in] bridge  A bridge pointer.
 *  @param[in] addr    A controller address.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 */
lagopus_result_t
channel_mgr_channel_delete(struct bridge *bridge, struct addrunion *addr);


/**
 * Look up all channels that have this dpid.
 *
 *  @param[in]  dpid    Datapath ID.
 *  @param[out] channel_list  Channels.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 */
lagopus_result_t
channel_mgr_channels_lookup_by_dpid(uint64_t dpid,
                                    struct channel_list **channel_list);

/**
 * Return true if there is even one alive channel with this dpid.
 *
 *  @param[in] dpid    Datapath ID.
 *
 *  @retval TRUE  Alive channel exists.
 *  @retval FAlSE Alive channel does not exist.
 */
bool
channel_mgr_has_alive_channel_by_dpid(uint64_t dpid);

/**
 * Iterate all channels with this dpid to call callback function.
 *
 *  @param[in]  dpid    Datapath ID.
 *  @param[in]  func    A callback function.
 *  @param[in]  val     A parameter for the callback function.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 *  @details Call this function for iterating a function to all same dpid channels.
 *
 */
lagopus_result_t
channel_mgr_dpid_iterate(uint64_t dpid,
                         lagopus_result_t (*func)(struct channel *chan, void *val), void *val);

/**
 * Return generation ID if it defined.
 *
 *  @param[in]  dpid    Datapath ID.
 *  @param[out] gen     Generation ID.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *  @retval LAGOPUS_RESULT_NOT_FOUND Fail, generation ID not found.
 *  @retval LAGOPUS_RESULT_NOT_DEFINED Fail, generation ID is not defined.
 *
 */
lagopus_result_t
channel_mgr_generation_id_get(uint64_t dpid, uint64_t *gen);

/**
 * Put generation ID into a channel list.
 *
 *  @param[in]  dpid    Datapath ID.
 *  @param[out] gen     Generation ID.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *  @retval LAGOPUS_RESULT_NOT_FOUND Fail, generation ID not found.
 *
 */
lagopus_result_t
channel_mgr_generation_id_set(uint64_t dpid, uint64_t gen);

/**
 * Look up a channel that has this channel ID.
 *
 *  @param[in]  dpid       Datapath ID.
 *  @param[in]  channel_id Channel ID.
 *  @param[out] channel    A channel.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *  @retval LAGOPUS_RESULT_NOT_FOUND Fail. channel not found.
 *
 */
lagopus_result_t
channel_mgr_channel_lookup_by_channel_id(uint64_t dpid,
    uint64_t channel_id, struct channel **channel);

#endif /* __CHANNEL_MGR_H__ */
