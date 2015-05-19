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
#include "lagopus/dpmgr.h"
#include "channel_mgr.h"
#include "agent.h"

static lagopus_mutex_t lock = NULL;
static lagopus_hashmap_t main_table = NULL;
static lagopus_hashmap_t dp_table = NULL;
static pthread_once_t initialized = PTHREAD_ONCE_INIT;
static struct event_manager *em;

#define MAKE_MAIN_HASH(key, ipaddr, addr, bridge) \
  snprintf((key), sizeof((key)), "%s:%02d:%s", (ipaddr), (addr)->family, \
           bridge_name_get((bridge)));

#define MAIN_KEY_LEN \
  INET6_ADDRSTRLEN+BRIDGE_MAX_NAME_LEN+5 /* 5 = len(":%02d") + 1 */

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
channel_mgr_channel_add_internal(struct bridge *bridge,
                                 uint64_t dpid, struct addrunion *addr,
                                 struct channel **channel) {
  lagopus_result_t ret;
  struct channel *chan = NULL;
  struct channel_list *chan_list = NULL;
  char ipaddr[INET6_ADDRSTRLEN+1];
  char key[MAIN_KEY_LEN];
  void *valptr = NULL;

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  ret = channel_mgr_channel_lookup(bridge, addr, &chan);
  if (ret == LAGOPUS_RESULT_OK && chan != NULL) {
    /* FIXME support auxiliary connections */
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  }

  chan = channel_alloc(addr, em, dpid);
  if (chan == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  lagopus_msg_info("channel allocated  %p\n", chan);

  if (addrunion_ipaddr_str_get(addr, ipaddr, sizeof(ipaddr)) == NULL) {
    channel_free(chan);
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  MAKE_MAIN_HASH(key, ipaddr, addr, bridge);
  valptr = chan;
  ret = lagopus_hashmap_add(&main_table, (void *)key, (void **)&valptr, false);
  if (ret != LAGOPUS_RESULT_OK) {
    channel_free(chan);
    return ret;
  }

  ret = lagopus_hashmap_find(&dp_table, (void *)bridge->dpid,
                             (void **)&chan_list);
  if (ret == LAGOPUS_RESULT_NOT_FOUND) {
    chan_list = channel_list_alloc();
    if (chan_list == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      goto done;
    }
    valptr = chan_list;
    ret = lagopus_hashmap_add(&dp_table, (void *)bridge->dpid, (void **)&valptr,
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
  if (ret != LAGOPUS_RESULT_OK) {
    channel_mgr_channel_delete(bridge, addr);
  } else {
    *channel = chan;
  }

  return ret;
}

lagopus_result_t
channel_mgr_channel_add(struct bridge *bridge, uint64_t dpid,
                        struct addrunion *addr) {
  lagopus_result_t ret;
  struct channel *chan = NULL;

  ret = channel_mgr_channel_add_internal(bridge, dpid, addr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }
  lagopus_msg_info("channel %p enabled\n", chan);
  channel_enable(chan);

  return ret;
}

lagopus_result_t
channel_mgr_channel_tls_add(struct bridge *bridge, uint64_t dpid,
                            struct addrunion *addr) {
  lagopus_result_t ret;
  struct channel *chan = NULL;

  ret = channel_mgr_channel_add_internal(bridge, dpid, addr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  lagopus_msg_info("channel %p protocol set\n", chan);
  ret = channel_protocol_set(chan, PROTOCOL_TLS);
  if (ret != LAGOPUS_RESULT_OK) {
    channel_mgr_channel_delete(bridge, addr);
    return ret;
  }
  lagopus_msg_info("channel %p enabled\n", chan);
  channel_enable(chan);

  return ret;
}

lagopus_result_t
channel_mgr_channel_lookup(struct bridge *bridge, struct addrunion *addr,
                           struct channel **chan) {
  char ipaddr[INET6_ADDRSTRLEN+1];
  char key[MAIN_KEY_LEN];

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  if (addrunion_ipaddr_str_get(addr, ipaddr, sizeof(ipaddr)) == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  MAKE_MAIN_HASH(key, ipaddr, addr, bridge);
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
channel_mgr_channel_delete(struct bridge *bridge, struct addrunion *addr) {
  char ipaddr[INET6_ADDRSTRLEN+1];
  char key[MAIN_KEY_LEN];
  struct channel *chan;
  lagopus_result_t ret;

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  if (addrunion_ipaddr_str_get(addr, ipaddr, sizeof(ipaddr)) == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  MAKE_MAIN_HASH(key, ipaddr, addr, bridge);
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
