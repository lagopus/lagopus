/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include "lagopus_apis.h"
#include "lagopus_thread.h"
#include "channel.h"
#include "event.h"
#include "lagopus/datastore.h"
#include "channel_mgr.h"
#include "agent.h"

static lagopus_mutex_t lock = NULL;
static lagopus_hashmap_t main_table = NULL;
static lagopus_hashmap_t dp_table = NULL;
static pthread_once_t initialized = PTHREAD_ONCE_INIT;
static struct event_manager *em;

#define MAKE_MAIN_HASH(key, ipaddr, addr, bridge_name) \
  snprintf((key), sizeof((key)), "%s:%02d:%s", (ipaddr), (addr)->family, \
           (bridge_name));

#define MAIN_KEY_LEN \
  INET6_ADDRSTRLEN+DATASTORE_BRIDGE_FULLNAME_MAX+5 /* 5 = len(":%02d") + 1 */

static void
channel_entry_free(void *arg) {
  channel_free(arg);
}

static void
channel_list_entry_free(void *arg) {
  channel_list_free(arg);
}

static void
initialize_internal(void) {
  lagopus_result_t ret;

  ret = lagopus_mutex_create(&lock);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("channel_mgr_initialize:lagopus_mutex_create");
  }

  ret = lagopus_hashmap_create(&main_table, LAGOPUS_HASHMAP_TYPE_STRING,
                               channel_entry_free);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("channel_mgr_initialize:lagopus_hashmap_create");
  }

  ret = lagopus_hashmap_create(&dp_table, LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                               channel_list_entry_free);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("channel_mgr_initialize:lagopus_hashmap_create");
  }

}

void
channel_mgr_initialize(void *arg) {
  em = (struct event_manager *) arg;
  pthread_once(&initialized, initialize_internal);
}

void
channel_mgr_finalize(void) {
  lagopus_result_t ret;

  ret = lagopus_hashmap_clear(&main_table, true);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  ret = lagopus_hashmap_clear(&dp_table, true);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return;
}

static lagopus_result_t
channel_delete_internal(const char *key) {
  lagopus_result_t ret;
  struct channel *chan;

  ret = lagopus_hashmap_find(&main_table, (void *)key, (void **)&chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  channel_disable(chan);

  ret = channel_free(chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  return lagopus_hashmap_delete(&main_table, (void *)key, NULL, false);
}

static void
sockaddr_to_addrunion(const struct sockaddr *saddr, struct addrunion *uaddr) {

  switch (saddr->sa_family) {
    case AF_INET:
      uaddr->addr4 = ((struct sockaddr_in *)saddr)->sin_addr;
      break;
    case AF_INET6:
      uaddr->addr6 = ((struct sockaddr_in6 *)saddr)->sin6_addr;
      break;
    default:
      lagopus_msg_warning("unknown address family %d\n", saddr->sa_family);
      return;
  }

  uaddr->family = saddr->sa_family;
}

static lagopus_result_t
channel_add_internal(const char *channel_name, struct addrunion *addr,
                     struct channel **channel) {
  lagopus_result_t ret;
  struct channel *chan = NULL;
  void *valptr = NULL;

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  ret = lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)&chan);
  if (ret == LAGOPUS_RESULT_OK && chan != NULL) {
    /* FIXME support auxiliary connections */
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  } else if (ret != LAGOPUS_RESULT_NOT_FOUND) {
    return ret;
  }

#define UNUSED_DPID 0
  chan = channel_alloc(addr, em, UNUSED_DPID);
  if (chan == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  lagopus_msg_debug(10, "channel allocated  %p\n", chan);

  valptr = chan;
  ret = lagopus_hashmap_add(&main_table, (void *)channel_name, (void **)&valptr,
                            false);
  if (ret != LAGOPUS_RESULT_OK) {
    channel_free(chan);
  }

  if (ret != LAGOPUS_RESULT_OK) {
    channel_delete_internal((const char *) channel_name);
  } else {
    *channel = chan;
  }

  return ret;
}

static lagopus_result_t
channel_set_dpid(const char *channel_name, uint64_t dpid) {
  lagopus_result_t ret;
  struct channel *chan = NULL;
  struct channel_list *chan_list = NULL;
  void *valptr = NULL;

  if (dpid == UNUSED_DPID) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  ret = lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)&chan);
  if (chan == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  if (channel_dpid_get(chan) != UNUSED_DPID) {
    return LAGOPUS_RESULT_BUSY;
  }

  ret = channel_dpid_set(chan, dpid);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = lagopus_hashmap_find(&dp_table, (void *)dpid, (void **)&chan_list);
  if (ret == LAGOPUS_RESULT_NOT_FOUND) {
    chan_list = channel_list_alloc();
    if (chan_list == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      goto done;
    }
    valptr = chan_list;
    ret = lagopus_hashmap_add(&dp_table, (void *)dpid, (void **)&valptr,
                              false);
    if (ret != LAGOPUS_RESULT_OK) {
      goto done;
    }
  } else if (ret != LAGOPUS_RESULT_OK) {
    goto done;
  }

  channel_id_set(chan, channel_list_channel_id_get(chan_list));
  ret = channel_list_insert(chan_list, chan);

done:

  return ret;
}

static lagopus_result_t
channel_unset_dpid(const char *channel_name) {
  uint64_t dpid;
  lagopus_result_t ret;
  struct channel *chan = NULL;
  struct channel_list *chan_list = NULL;

  ret = lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)&chan);
  if (chan == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  if (channel_is_alive(chan) == true) {
    return LAGOPUS_RESULT_BUSY;
  }

  dpid = channel_dpid_get(chan);
  if (dpid == UNUSED_DPID) {
    return LAGOPUS_RESULT_OK;
  }

  ret = lagopus_hashmap_find(&dp_table, (void *)dpid, (void **)&chan_list);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = channel_list_delete(chan_list, chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = channel_dpid_set(chan, UNUSED_DPID);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  return ret;
}

static lagopus_result_t
channel_mgr_channel_add_internal(const char *bridge_name,
                                 uint64_t dpid, struct addrunion *addr,
                                 struct channel **channel) {
  lagopus_result_t ret;
  struct channel *chan = NULL;
  char ipaddr[INET6_ADDRSTRLEN+1];
  char key[MAIN_KEY_LEN];

  if (addrunion_ipaddr_str_get(addr, ipaddr, sizeof(ipaddr)) == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  MAKE_MAIN_HASH(key, ipaddr, addr, bridge_name);
  ret = channel_add_internal(key, addr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = channel_set_dpid(key, dpid);
  if (ret != LAGOPUS_RESULT_OK) {
    channel_mgr_channel_delete(bridge_name, addr);
  } else {
    *channel = chan;
  }

  return ret;
}

lagopus_result_t
channel_mgr_channel_add(const char *bridge_name, uint64_t dpid,
                        struct addrunion *addr) {
  lagopus_result_t ret;
  struct channel *chan = NULL;

  ret = channel_mgr_channel_add_internal(bridge_name, dpid, addr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }
  lagopus_msg_debug(10, "channel %p enabled\n", chan);
  channel_enable(chan);

  return ret;
}

lagopus_result_t
channel_mgr_channel_tls_add(const char *bridge_name, uint64_t dpid,
                            struct addrunion *addr) {
  lagopus_result_t ret;
  struct channel *chan = NULL;

  ret = channel_mgr_channel_add_internal(bridge_name, dpid, addr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  lagopus_msg_debug(10, "channel %p protocol set\n", chan);
  ret = channel_protocol_set(chan, PROTOCOL_TLS);
  if (ret != LAGOPUS_RESULT_OK) {
    channel_mgr_channel_delete(bridge_name, addr);
    return ret;
  }
  lagopus_msg_debug(10, "channel %p enabled\n", chan);
  channel_enable(chan);

  return ret;
}

lagopus_result_t
channel_mgr_channel_lookup(const char *bridge_name, struct addrunion *addr,
                           struct channel **chan) {
  char ipaddr[INET6_ADDRSTRLEN+1];
  char key[MAIN_KEY_LEN];

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  if (addrunion_ipaddr_str_get(addr, ipaddr, sizeof(ipaddr)) == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  MAKE_MAIN_HASH(key, ipaddr, addr, bridge_name);
  return lagopus_hashmap_find(&main_table, (void *)key, (void **)chan);
}

lagopus_result_t
channel_mgr_channels_lookup_by_dpid(uint64_t dpid,
                                    struct channel_list **chan_list) {
  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  return lagopus_hashmap_find(&dp_table, (void *)dpid, (void **)chan_list);
}

lagopus_result_t
channel_mgr_channel_lookup_by_name(const char *channel_name,
                                   struct channel **chan) {

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  return lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)chan);
}


struct channel_id_vars {
  uint64_t channel_id;
  struct channel *channel;
};

static lagopus_result_t
channel_id_eq(struct channel *chan, void *val) {
  struct channel_id_vars *v = val;

  if (channel_id_get(chan) == v->channel_id) {
    v->channel = chan;
    return 1;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_channel_lookup_by_channel_id(uint64_t dpid,
    uint64_t channel_id, struct channel **chan) {
  lagopus_result_t ret;
  struct channel_id_vars v;

  v.channel_id = channel_id;
  v.channel = NULL;

  channel_mgr_dpid_iterate(dpid, channel_id_eq, &v);
  if (v.channel == NULL) {
    ret = LAGOPUS_RESULT_NOT_FOUND;
  } else {
    ret = LAGOPUS_RESULT_OK;
  }
  *chan = v.channel;

  return ret;
}

bool
channel_mgr_has_alive_channel_by_dpid(uint64_t dpid) {
  int alive;
  lagopus_result_t ret;
  struct channel_list *chan_list;

  ret = channel_mgr_channels_lookup_by_dpid(dpid, &chan_list);
  if (ret != LAGOPUS_RESULT_OK) {
    return false;
  }

  alive = channel_list_get_alive_num(chan_list);
  if (alive == 0) {
    return false;
  }

  return true;
}

lagopus_result_t
channel_mgr_dpid_iterate(uint64_t dpid,
                         lagopus_result_t (*func)(struct channel *chan, void *val), void *val) {
  lagopus_result_t ret;
  struct channel_list *chan_list;

  ret = channel_mgr_channels_lookup_by_dpid(dpid, &chan_list);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = channel_list_iterate(chan_list, func, (void *) val);

  return ret;
}

lagopus_result_t
channel_mgr_channel_delete(const char *bridge_name, struct addrunion *addr) {
  char ipaddr[INET6_ADDRSTRLEN+1];
  char key[MAIN_KEY_LEN];

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  if (addrunion_ipaddr_str_get(addr, ipaddr, sizeof(ipaddr)) == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  MAKE_MAIN_HASH(key, ipaddr, addr, bridge_name);
  return channel_delete_internal((const char *) key);
}

lagopus_result_t
channel_mgr_generation_id_get(uint64_t dpid, uint64_t *gen) {
  lagopus_result_t ret;
  struct channel_list *chan_list;

  ret = channel_mgr_channels_lookup_by_dpid(dpid, &chan_list);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("channel head is not found. dpid: %ld\n", dpid);
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  if (channel_list_generation_id_is_defined(chan_list) == false) {
    return LAGOPUS_RESULT_NOT_DEFINED;
  }

  *gen = channel_list_generation_id_get(chan_list);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_generation_id_set(uint64_t dpid, uint64_t gen) {
  lagopus_result_t ret;
  struct channel_list *chan_list;

  ret = channel_mgr_channels_lookup_by_dpid(dpid, &chan_list);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("channel head is not found. dpid: %ld\n", dpid);
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  channel_list_generation_id_set(chan_list, gen);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_channel_create(const char *channel_name,
                           lagopus_ip_address_t *dst_addr,
                           uint16_t dst_port, lagopus_ip_address_t *src_addr, uint16_t src_port,
                           datastore_channel_protocol_t protocol) {
  lagopus_result_t ret;
  struct channel *chan = NULL;
  struct sockaddr *sa;
  struct sockaddr_storage ss = {0};
  struct addrunion daddr = {0}, saddr = {0};
  enum channel_protocol cprotocol;


  sa = (struct sockaddr *) &ss;
  ret = lagopus_ip_address_sockaddr_get(dst_addr, &sa);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }
  sockaddr_to_addrunion(sa, &daddr);

  ret = channel_add_internal(channel_name, &daddr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = channel_port_set(chan, dst_port);
  if (ret != LAGOPUS_RESULT_OK) {
    goto fail;
  }

  if (src_addr != NULL) {
    ret = lagopus_ip_address_sockaddr_get(src_addr, &sa);
    if (ret != LAGOPUS_RESULT_OK) {
      goto fail;
    }
    sockaddr_to_addrunion(sa, &saddr);

    ret = channel_local_addr_set(chan, &saddr);
    if (ret != LAGOPUS_RESULT_OK) {
      goto fail;
    }
  }

  ret = channel_local_port_set(chan, src_port);
  if (ret != LAGOPUS_RESULT_OK) {
    goto fail;
  }

  switch (protocol) {
    case DATASTORE_CHANNEL_PROTOCOL_TCP:
      cprotocol = PROTOCOL_TCP;
      break;
    case DATASTORE_CHANNEL_PROTOCOL_TLS:
      cprotocol = PROTOCOL_TLS;
      break;
    default:
      return LAGOPUS_RESULT_INVALID_ARGS;
  }

  ret = channel_protocol_set(chan, cprotocol);
  if (ret != LAGOPUS_RESULT_OK) {
    goto fail;
  }

  return LAGOPUS_RESULT_OK;

fail:
  channel_delete_internal(channel_name);
  return ret;
}

lagopus_result_t
channel_mgr_channel_destroy(const char *channel_name) {
  int i;
  lagopus_result_t ret;

  for (i = 0; i < 10; i++) {
    ret = channel_delete_internal(channel_name);
    if (ret == LAGOPUS_RESULT_BUSY) {
      usleep(10000/* 10msec sleep*/);
    } else {
      break;
    }
  }

  return ret;
}

lagopus_result_t
channel_mgr_channel_start(const char *channel_name) {
  lagopus_result_t ret;
  struct channel *chan;

  ret = lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)&chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  if (channel_dpid_get(chan) == UNUSED_DPID) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  channel_enable(chan);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_channel_stop(const char *channel_name) {
  lagopus_result_t ret;
  struct channel *chan;

  ret = lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)&chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  channel_disable(chan);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_controller_set(const char *channel_name,
                           datastore_controller_role_t role,
                           datastore_controller_connection_type_t type) {
  uint32_t ofp_role, prev_role;
  lagopus_result_t ret;
  struct channel *chan;

  ret = channel_mgr_channel_lookup_by_name(channel_name, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  prev_role = channel_role_get(chan);

  switch (role) {
    case DATASTORE_CONTROLLER_ROLE_EQUAL:
      ofp_role = OFPCR_ROLE_EQUAL;
      break;
    case DATASTORE_CONTROLLER_ROLE_MASTER:
      ofp_role = OFPCR_ROLE_MASTER;
      break;
    case DATASTORE_CONTROLLER_ROLE_SLAVE:
      ofp_role = OFPCR_ROLE_SLAVE;
      break;
    default:
      return LAGOPUS_RESULT_INVALID_ARGS;
  }

  channel_role_set(chan, ofp_role);

  if (type == DATASTORE_CONTROLLER_CONNECTION_TYPE_MAIN) {
    ret = channel_auxiliary_set(chan, false);
  } else if (type == DATASTORE_CONTROLLER_CONNECTION_TYPE_AUXILIARY) {
    ret = channel_auxiliary_set(chan, true);
  } else {
    channel_role_set(chan, prev_role);
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret != LAGOPUS_RESULT_OK) {
    channel_role_set(chan, prev_role);
    return ret;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_channel_dpid_set(const char *channel_name, uint64_t dpid) {
  return channel_set_dpid(channel_name, dpid);
}

lagopus_result_t
channel_mgr_channel_dpid_get(const char *channel_name, uint64_t *dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *chan;

  ret = lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)&chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  *dpid = channel_dpid_get(chan);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_channel_dpid_unset(const char *channel_name) {
  return channel_unset_dpid(channel_name);
}

lagopus_result_t
channel_mgr_channel_local_addr_get(const char *channel_name,
                                   lagopus_ip_address_t **local_addr) {
  char ipaddr[INET6_ADDRSTRLEN+1];
  lagopus_result_t ret;
  struct channel *chan;
  struct addrunion addr = {0};

  ret = channel_mgr_channel_lookup_by_name(channel_name, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = channel_local_addr_get(chan, &addr);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  if (addrunion_ipaddr_str_get(&addr, ipaddr, sizeof(ipaddr)) == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  ret = lagopus_ip_address_create(ipaddr,
                                  addrunion_af_get(&addr) == AF_INET ? true : false, local_addr);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  return ret;
}

lagopus_result_t
channel_mgr_channel_local_port_get(const char *channel_name,
                                   uint16_t *local_port) {
  lagopus_result_t ret;
  struct channel *chan;

  ret = channel_mgr_channel_lookup_by_name(channel_name, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  return channel_local_port_get(chan, local_port);
}

lagopus_result_t
channel_mgr_channel_connection_status_get(const char *channel_name,
    datastore_channel_status_t *status) {
  lagopus_result_t ret;
  struct channel *chan;

  ret = channel_mgr_channel_lookup_by_name(channel_name, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  if (channel_is_alive(chan)) {
    *status = DATASTORE_CHANNEL_CONNECTED;
  } else {
    *status = DATASTORE_CHANNEL_DISONNECTED;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_channel_role_get(const char *channel_name,
                             datastore_controller_role_t *role) {
  uint32_t ofp_role;
  lagopus_result_t ret;
  struct channel *chan;


  ret = channel_mgr_channel_lookup_by_name(channel_name, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ofp_role = channel_role_get(chan);

  switch (ofp_role) {
    case OFPCR_ROLE_EQUAL:
      *role = DATASTORE_CONTROLLER_ROLE_EQUAL;
      break;
    case OFPCR_ROLE_MASTER:
      *role = DATASTORE_CONTROLLER_ROLE_MASTER;
      break;
    case OFPCR_ROLE_SLAVE:
      *role = DATASTORE_CONTROLLER_ROLE_SLAVE;
      break;
    default:
      *role = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
      break;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_ofp_version_get(const char *channel_name, uint8_t *version) {
  lagopus_result_t ret;
  struct channel *chan;

  ret = channel_mgr_channel_lookup_by_name(channel_name, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  *version = channel_version_get(chan);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_channel_is_alive(const char *channel_name, bool *b) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *chan;

  ret = lagopus_hashmap_find(&main_table, (void *)channel_name, (void **)&chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  *b = channel_session_is_alive(chan);

  return LAGOPUS_RESULT_OK;
}
