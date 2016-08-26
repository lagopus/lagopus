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

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

/**
 * @file        channel.h
 */


#include "lagopus/bridge.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_handler.h"
#include "ofp_role.h"

struct ofp_header;
struct channel;
struct channel_list;
enum channel_event;

/* Here is channel API to config system. */

enum channel_protocol {
  PROTOCOL_TCP  = 0,
  PROTOCOL_TLS  = 1,
  PROTOCOL_TCP6 = 2,
  PROTOCOL_TLS6 = 3,
};

/* decl channelq_t */
struct channelq_data {
  struct channel *channel;
  struct pbuf *pbuf;
};

#define SEC_TO_NSEC(a)  ((a) * 1000LL * 1000LL * 1000LL)

typedef LAGOPUS_BOUND_BLOCK_Q_DECL(channelq,
                                   struct channelq_data *) channelq_t;

/**
 * Destroy channelq_data.
 *
 *  @param[in] cdata channel queue data.
 *
 *  @details Call this function when finished processing channelq data
 *  in ofp handler.
 */
void
channelq_data_destroy(struct channelq_data *cdata);

/* OpenFlow channel. */

/**
 * Allocate a channel object.
 *
 *  @param[in] controller  A controller address.
 *  @param[in] dpid        Datapath id.
 *
 *  @retval !NULL Succeeded, allocated new channel pointer.
 *  @retval NULL  Failed, maybe no memory.
 *
 *  @details Call this function for getting a new channel object.
 */
struct channel *
channel_alloc(lagopus_ip_address_t *controller, uint64_t dpid);

/**
 * Free a channel object.
 *
 *  @param[in] channel A channel pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded, the channel deleted.
 *  @retval LAGOPUS_RESULT_BUSY Failed, this channel is used yet.
 *  @retval LAGOPUS_RESULT_INVALID_ARGS Failed.
 *
 *  @details Call this function for freeing a channel object.
 *  Assume that a channel is not running.
 *  In this function, a channel object was removed from channel list.
 *
 */
lagopus_result_t
channel_free(struct channel *channel);

/* Channel getter/seter. */

/**
 * Return XID with incrementing it.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval XID
 *
 */
uint32_t
channel_xid_get(struct channel *channel);

/**
 * Return XID with incrementing it.
 * No lock version.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval XID
 *
 */
uint32_t
channel_xid_get_nolock(struct channel *channel);

/**
 * Set XID in channel.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] xid      XID.
 *
 */
void
channel_xid_set(struct channel *channel, uint32_t xid);

/**
 * Return protocol version.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Protocol version
 *
 */
uint8_t
channel_version_get(struct channel *channel);

/**
 * Return protocol version.
 * No lock version.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Protocol version
 *
 */
uint8_t
channel_version_get_nolock(struct channel *channel);

/**
 * Set protocol version into channel.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in]  Version.
 *
 */
void
channel_version_set(struct channel *channel, uint8_t version);

/**
 * Set protocol version into channel.
 * No lock version.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in]  Version.
 *
 */
void
channel_version_set_nolock(struct channel *channel, uint8_t version);

/**
 * Return session in channel.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Session pointer.
 *
 */
lagopus_session_t
channel_session_get(struct channel *channel);

/**
 * Set a session into channel.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in]  session A session.
 *
 */
void
channel_session_set(struct channel *channel, lagopus_session_t session);

/**
 * Return role in channel.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Channel role.
 *
 */
uint32_t
channel_role_get(struct channel *channel);

/**
 * Set a role into channel.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] role     Role.
 *
 */
void
channel_role_set(struct channel *channel, uint32_t role);

/**
 * Set datapath id into channel.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] Datapath ID.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_dpid_set(struct channel *channel, uint64_t dpid);

/**
 * Return datapath id in channel.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Datapath ID.
 *
 */
uint64_t
channel_dpid_get(struct channel *channel);

/**
 * Return datapath id in channel.
 * No lock version.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Datapath ID.
 *
 */
uint64_t
channel_dpid_get_nolock(struct channel *channel);

/**
 * Return channel ID in channel.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Channel ID role.
 *
 */
uint64_t
channel_id_get(struct channel *channel);

/**
 * Set channel ID into channel.
 *
 *  @param[in] channel     A channel pointer.
 *  @param[in] channel_id  Channel ID
 *
 */
void
channel_id_set(struct channel *channel, uint64_t channel_id);

/**
 * Return a pbuf from unused pbuf list.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] size     Needed pbuf size.
 *
 *  @retval Pbuf pointer.
 *
 */
struct pbuf *
channel_pbuf_list_get(struct channel *channel, size_t size);

/**
 * Return a pbuf from unused pbuf list.
 * No lock version.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] size     Needed pbuf size.
 *
 *  @retval Pbuf pointer.
 *
 */
struct pbuf *
channel_pbuf_list_get_nolock(struct channel *channel, size_t size);

/**
 * Push back a used pbuf into unused pbuf list.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] pbuf     A pbuf pointer.
 *
 */
void
channel_pbuf_list_unget(struct channel *channel, struct pbuf *pbuf);

/**
 * Push back a used pbuf into unused pbuf list.
 * No lock version.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] pbuf     A pbuf pointer.
 *
 */
void
channel_pbuf_list_unget_nolock(struct channel *channel, struct pbuf *pbuf);

/**
 * Send a packet to a controller.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] pbuf     A pbuf pointer.
 *
 */
void
channel_send_packet(struct channel *channel, struct pbuf *pbuf);

/**
 * Send a packet to a controller by event manager (nolock).
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] pbuf     A pbuf pointer.
 *
 */
void
channel_send_packet_by_event_nolock(struct channel *channel, struct pbuf *pbuf);

/**
 * Send a packet to a controller by event manager.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] pbuf     A pbuf pointer.
 *
 */
void
channel_send_packet_by_event(struct channel *channel, struct pbuf *pbuf);

/**
 * Send packets to a controller.
 *
 *  @param[in] channel    A channel pointer.
 *  @param[in] pbuf_list  Pbuf list pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded, sent packets.
 *  @retval LAGOPUS_RESULT_INVALID_ARGS Failed,
 *
 */
lagopus_result_t
channel_send_packet_list(struct channel *channel,
                         struct pbuf_list *pbuf_list);

/**
 * Return a controller address setting in a channel.
 * No lock version.
 *
 *  @param[in]  channel    A channel pointer.
 *  @param[out] addr  A controller address.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_addr_get(struct channel *channel, lagopus_ip_address_t **addr);

/**
 * Set a protocol type into a channel.
 *
 *  @param[in]  channel          A channel pointer.
 *  @param[in]  channel_protocol A channel protocol type.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_protocol_set(struct channel *channel, enum channel_protocol protocol);

/**
 * Set a default protocol type(PROTOCOL_TCP) into a channel.
 *
 *  @param[in]  channel          A channel pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_protocol_unset(struct channel *channel);

/**
 * Return a protocol type in a channel.
 *
 *  @param[in]  channel          A channel pointer.
 *  @param[out] channel_protocol A channel protocol type.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_protocol_get(struct channel *channel, enum channel_protocol *protocol);

/**
 * Set a remote port number into a channel.
 *
 *  @param[in]  channel          A channel pointer.
 *  @param[in]  port             A remote port number.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 *	@details Call this function for setting a controller tcp port number
 *	with tcp/ssl connection.
 */
lagopus_result_t
channel_port_set(struct channel *channel, uint16_t port);

/**
 * Set a default remote port number(6633) into a channel.
 *
 *  @param[in]  channel  A channel pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_port_unset(struct channel *channel);

/**
 * Return a port number in a channel.
 *
 *  @param[in]  channel  A channel pointer.
 *  @param[out] port     A channel port number.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_port_get(struct channel *channel, uint16_t *port);

/**
 * Set a local port number into a channel.
 *
 *  @param[in]  channel  A channel pointer.
 *  @param[in]  port     A local port number.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 *	@details Call this function for setting a local tcp port number
 *	with tcp/ssl connection.
 *  If setting a local port number, call to bind(2) when session craeted.
 *
 */
lagopus_result_t
channel_local_port_set(struct channel *channel, uint16_t port);

/**
 * Set zero for a local port number into a channel.
 *
 *  @param[in]  channel  A channel pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_local_port_unset(struct channel *channel);

/**
 * Return a local port number in a channel.
 *
 *  @param[in]  channel  A channel pointer.
 *  @param[out] port     A channel local port number.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_local_port_get(struct channel *channel, uint16_t *port);

/**
 * Set a local address into a channel.
 *
 *  @param[in]  channel    A channel pointer.
 *  @param[in]  addr  A local address.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *  @retval LAGOPUS_RESULT_BUSY Failed, a channel session is alive.
 *
 *	@details Call this function for setting a local address
 *	with tcp/ssl connection.
 *  If setting a local address, call to bind(2) when session craeted.
 *
 */
lagopus_result_t
channel_local_addr_set(struct channel *channel, lagopus_ip_address_t *addr);

/**
 * Set INADDR_ANY for a local address into a channel.
 *
 *  @param[in]  channel  A channel pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *  @retval LAGOPUS_RESULT_BUSY Failed, a channel session is alive.
 *
 */
lagopus_result_t
channel_local_addr_unset(struct channel *);

/**
 * Return a local address in a channel.
 *
 *  @param[in]  channel    A channel pointer.
 *  @param[out] addr  A channel local address.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_local_addr_get(struct channel *channel, lagopus_ip_address_t **addr);

/**
 * Put a part of multipart message into multipart list.
 *
 *  @param[in]  channel    A channel pointer.
 *  @param[in]  pbuf       A pbuf.
 *  @param[in]  xid_header A ofp_header.
 *  @param[in]  mtype      A multi part type.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *  @retval LAGOPUS_RESULT_OFP_ERROR Failed, received xid has already used.
 *  @retval LAGOPUS_RESULT_NO_MEMORY Failed, no memory or number of different
 *  multipart messages is over CHANNEL_SIMULTANEOUS_MULTIPART_MAX(default 16).
 *
 */
lagopus_result_t
channel_multipart_put(struct channel *channel, struct pbuf *pbuf,
                      struct ofp_header *xid_header, uint16_t mtype);


/**
 * Return a pbuf that all multipart messages gathered into.
 *
 *  @param[in]  channel    A channel pointer.
 *  @param[out] pbuf       A pbuf of all multipert messages.
 *  @param[in]  xid_header A ofp_header.
 *  @param[in]  mtype      A multi part type.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *  @retval LAGOPUS_RESULT_NOT_FOUND Failed, not found multipart messages.
 *  @retval LAGOPUS_RESULT_NO_MEMORY Failed, no memory.
 *
 */
lagopus_result_t
channel_multipart_get(struct channel *channel, struct pbuf **pbuf,
                      struct ofp_header *xid_header, uint16_t mtype);

/**
 * Return auxiliary id.
 *
 *  @param[in]  channel    A channel pointer.
 *
 *  @retval Auxiliary id.
 *
 */
uint8_t
channel_auxiliary_id_get(struct channel *channel);

/**
 * Return number of used multipath entries.
 *
 *  @param[in]  channel    A channel pointer.
 *
 *  @retval Number of used multipath entries.
 *
 */
int
channel_multipart_used_count_get(struct channel *channel);

/**
 * Return a channel(TEST USE ONLY).
 *
 *  @param[in]  ipaddr  IP address.
 *  @param[in]  port    A port number.
 *  @param[in]  dpid    Data path ID.
 *
 *  @retval A channel.
 *
 */
struct channel *
channel_alloc_ip4addr(const char *ipaddr, const char *port, uint64_t dpid);

/**
 * Increment a channel reference counter.
 *
 *  @param[in]  channel  A channel pointer.
 *
 */
void
channel_refs_get(struct channel *channel);

/**
 * Decrement a channel reference counter.
 *
 *  @param[in]  channel  A channel pointer.
 *
 */
void
channel_refs_put(struct channel *channel);


/**
 * Allocate a channel_list object.
 *
 *  @retval !NULL Succeeded A channel list object.
 *  @retval NULL  Failed, no memory.
 *
 *  @details Channel list is used for putting together same dpid channels.
 *
 */
struct channel_list *
channel_list_alloc(void);

/**
 * Free a channel_list object.
 *
 *  @param[in]  channel_list  A channel list pointer.
 *
 *  @details Assume that all channels in this channel list have been freed.
 *  When channel_free() was called with a channel, that channel removed
 *  from channel list at same time.
 *
 */
void
channel_list_free(struct channel_list *channel_list);

/**
 * Insert a channel to channel list.
 *
 *  @param[in]  channel_list  A channel list pointer.
 *  @param[in]  channel       A channel pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_list_insert(struct channel_list *channel_list,
                    struct channel *channel);

/**
 * Delete a channel from channel list.
 *
 *  @param[in]  channel_list  A channel list pointer.
 *  @param[in]  channel       A channel pointer.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 */
lagopus_result_t
channel_list_delete(struct channel_list *channel_list,
                    struct channel *channel);

/**
 * Iterate all channels in a channel list to call callback function.
 *
 *  @param[in]  channel_list  A channel list pointer.
 *  @param[in]  func          A callback function.
 *  @param[in]  val           A parameter for the callback function.
 *
 *  @retval LAGOPUS_RESULT_OK Succeeded.
 *
 *  @details Call this function for iterating a function to all same dpid channels.
 *
 */
lagopus_result_t
channel_list_iterate(struct channel_list *channel_list,
                     lagopus_result_t (*func)(struct channel *channel, void *val), void *val);

/**
 * Return a channel is alive or not.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval TRUE  Alive.
 *  @retval FALSE Dead.
 *
 */
bool
channel_is_alive(struct channel *channel);

/**
 * Return a session is alive or not in channel.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval TRUE  Alive.
 *  @retval FALSE Dead.
 *
 */
bool
channel_session_is_alive(struct channel *channel);

/**
 * Return channel role mask.
 *
 *  @param[in] channel    A channel pointer.
 *  @param[in] role_mask  A channel role mask.
 *
 *
 */
void
channel_role_mask_get(struct channel *channel,
                      struct ofp_async_config *role_mask);

/**
 * Put channel role mask into a channel.
 *
 *  @param[in] channel    A channel pointer.
 *  @param[in] role_mask  A channel role mask.
 *
 *
 */
void
channel_role_mask_set(struct channel *channel,
                      struct ofp_async_config *role_mask);

/**
 * Return number of alive channels in a channel list.
 *
 *  @param[in] channel_list  A channel list.
 *
 *  @retval Number of alive channels.
 *
 */
int
channel_list_get_alive_num(struct channel_list *channel_list);

/**
 * Lock a channel list.
 *
 *  @param[in] channel_list  A channel list.
 *
 */
void
channel_list_lock(struct channel_list *channel_list);

/**
 * Unlock a channel list.
 *
 *  @param[in] channel_list  A channel list.
 *
 */
void
channel_list_unlock(struct channel_list *chan_head);

/**
 * Return that a channel has role matching type and reason or not.
 *
 *  @param[in] channel    A channel pointer.
 *  @param[in] type       ofp_type.
 *  @param[in] reason     Reason of each openflow messages.
 *
 *  @retval TRUE  Role is matched.
 *  @retval FALSE Role is not matched.
 *
 */
bool
channel_role_channel_check_mask(struct channel *channel,
                                uint8_t type, uint8_t reason);

/**
 * Return generation ID is defined or not.
 *
 *  @param[in] channel_list    A channel list.
 *
 *  @retval TRUE  Generation ID is defined.
 *  @retval FALSE Generation ID is not defined.
 *
 */
bool
channel_list_generation_id_is_defined(struct channel_list *channel_list);

/**
 * Return generation ID.
 *
 *  @param[in] channel_list    A channel list.
 *
 *  @retval Generation ID.
 *
 */
uint64_t
channel_list_generation_id_get(struct channel_list *channel_list);

/**
 * Put generation ID into a channel list.
 *
 *  @param[in] channel_list    A channel list.
 *  @param[in] gen             Generation ID.
 *
 */
void
channel_list_generation_id_set(struct channel_list *channel_list,
                               uint64_t gen);

/**
 * Return channel ID with incrementing it.
 *
 *  @param[in] channel_list    A channel list.
 *
 *  @retval incremented channel ID.
 *
 */
uint64_t
channel_list_channel_id_get(struct channel_list *chanel_list);

/**
 * Enable a channel to run.
 *
 *  @param[in] channel    A channel pointer.
 *
 *  @details Call this function for starting a channel session.
 *
 */
void
channel_enable(struct channel *channel);

/**
 * Disable a channel to run.
 *
 *  @param[in] channel    A channel pointer.
 *
 *  @details Call this function for stopping a channel session.
 *  If channel will be free, call this function before.
 *
 */
void
channel_disable(struct channel *channel);

/**
 * Put OpenFlow_Hello_Received state int a channel.
 *
 *  @param[in] channel    A channel pointer.
 *
 *  @details Call this function after receiving ofp_hello message
 *  from a controller.
 *
 */
void
channel_hello_received_set(struct channel *channel);

/**
 * Set auxiliary connection type into channel.
 *
 *  @param[in] channel  A channel pointer.
 *  @param[in] is_auxiliary   Auxiliary connection or not.
 *
 */
lagopus_result_t
channel_auxiliary_set(struct channel *channel, bool is_auxiliary);

/**
 * Return auxiliary connectin or not.
 *
 *  @param[in] channel  A channel pointer.
 *
 *  @retval Auxiliary connectin or not.
 *
 */
lagopus_result_t
channel_auxiliary_get(struct channel *channel, bool *is_auxiliary);

#ifndef USE_EVENT
/**
 * Read packets from channel and process.
 *
 *  @param[in] channel  A channel pointer.
 *
 */
void
channel_read(struct channel *channel);

/**
 * Write packets to channel.
 *
 *  @param[in] channel  A channel pointer.
 *
 */
void
channel_write(struct channel *channel);

#endif

#endif /*__CHANNEL_H__ */
