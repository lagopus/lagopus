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

#include "lagopus_apis.h"
#include "lagopus_thread.h"
#include "channel.h"
#include "lagopus/datastore.h"
#include "channel_mgr.h"
#include "agent.h"

static lagopus_mutex_t lock = NULL;
static lagopus_hashmap_t main_table = NULL;
static lagopus_hashmap_t dp_table = NULL;
static pthread_once_t initialized = PTHREAD_ONCE_INIT;
static bool run = false;
static lagopus_session_t event_w, event_r;
static lagopus_callout_task_t channel_free_task;

#define UNUSED_DPID (0)
#define MAX_CHANNELES (1024)
#define POLL_TIMEOUT  (1000) /* 1sec */
#define MAKE_MAIN_HASH(key, ipaddr, addr, bridge_name) \
  snprintf((key), sizeof((key)), "%s:%02d:%s", (ipaddr), (addr)->sa_family, \
           (bridge_name));
#define MAIN_KEY_LEN \
  (INET6_ADDRSTRLEN+DATASTORE_BRIDGE_FULLNAME_MAX+5) /* 5 = len(":%02d") + 1 */

static void
channel_entry_free(void *arg) {
  channel_disable(arg);
  channel_free(arg);
}
struct session_set {
  int n_sessions;
  lagopus_session_t s[MAX_CHANNELES+1]; /* +1 = a session for event trigger */
};

static bool
channel_session_gather(__UNUSED void *key, void *val, __UNUSED lagopus_hashentry_t he, void *arg) {
  lagopus_session_t s;
  struct channel *chan = (struct channel *) val;
  struct session_set *ss = (struct session_set *) arg;

  lagopus_msg_debug(10, "called, channel:%p.\n", chan);
  s = channel_session_get(chan);
  if (s != NULL && ss->n_sessions < (int) sizeof(ss->s)) {
    bool r, w;
    (void) session_is_read_on(s, &r);
    (void) session_is_write_on(s, &w);
    lagopus_msg_debug(10, "channel:%p, session:%p, r:%d, w:%d.\n", chan, s, (int) r, (int) w);
    if (w || r) {
      ss->s[ss->n_sessions++] = s;
    }

  } else if (ss->n_sessions >= (int) sizeof(ss->s)) {
    lagopus_msg_warning("session set overflow %d.\n", ss->n_sessions);
    return false;
  }

  return true;
}

static bool
channel_process(__UNUSED void *key, void *val, __UNUSED lagopus_hashentry_t he, void *arg) {
  int *cnt = (int *) arg;
  lagopus_session_t s;
  struct channel *channel = (struct channel *) val;

  channel_refs_get(channel);
  s = channel_session_get(channel);
  if (s != NULL) {
    bool r = false, w = false;

    (void) session_is_readable(s, &r);
    if (r) {
      lagopus_msg_debug(10, "channel_read called:%p\n", channel);
      session_read_event_unset(s);
      channel_read(channel);
    }

    (void) session_is_writable(s, &w);
    if (w) {
      lagopus_msg_debug(10, "channel_write called:%p\n", channel);
      session_write_event_unset(s);
      channel_write(channel);
    }

    if (r | w) {
      (*cnt)--;
    }
  }
  channel_refs_put(channel);

  if (*cnt == 0) {
    /* all sessions were proccessed. */
    return false;
  }

  return true;
}

lagopus_result_t
channel_mgr_loop(__UNUSED const lagopus_thread_t *t, __UNUSED void *arg) {
  struct session_set ss = {0, {NULL}};
  lagopus_result_t ret;

  lagopus_msg_debug(10, "called, run: %d\n", run);
  while(run) {
    session_read_event_set(event_r);
    ss.s[0] = event_r;
    ss.n_sessions = 1;

    ret = lagopus_hashmap_iterate(&main_table, channel_session_gather, &ss);
    if (ret != LAGOPUS_RESULT_OK && ret != LAGOPUS_RESULT_ITERATION_HALTED) {
      lagopus_perror(ret);
      return ret;
    }

    lagopus_msg_debug(10, "%d sessions enabled.\n", ss.n_sessions);
    ret = session_poll(ss.s, ss.n_sessions, POLL_TIMEOUT);
    lagopus_msg_debug(10, "session_poll() return: %d\n", (int) ret);
    if (ret > 0) {
      bool readable;

      (void) session_is_readable(event_r, &readable);
      lagopus_msg_debug(10, "event readable:%d\n", readable);
      if (readable) { /* event occured */
        char buf[BUFSIZ];
        /* drain event messages. */
        session_read(event_r, buf, sizeof(buf));
        ret--;
      }

      if (ret) {
        (void) lagopus_hashmap_iterate(&main_table, channel_process, &ret);
      }
    } else if (ret == LAGOPUS_RESULT_TIMEDOUT) {
      lagopus_msg_debug(10, "channel mgr loop timeouted.\n");
    } else if (ret == LAGOPUS_RESULT_INTERRUPTED) {
      lagopus_msg_info("channel mgr loop interrupted.\n");
    } else {
      lagopus_perror(ret);
    }
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
periodical_channel_entry_free(struct channel *channel, __UNUSED void *arg) {
  lagopus_result_t ret;

  lagopus_msg_debug(10, "channel free(%p)\n", channel);
  ret = channel_free(channel);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("can't free channel(%p), %s.\n",
                    channel, lagopus_error_get_string(ret));
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
periodical_channel_free(__UNUSED void *arg) {
  channel_mgr_dpid_iterate(UNUSED_DPID, periodical_channel_entry_free, NULL);
  return LAGOPUS_RESULT_OK;
}

static void
channel_list_entry_free(void *arg) {
  lagopus_result_t ret;

  ret = channel_list_iterate(arg, periodical_channel_entry_free, NULL);
  if (ret != LAGOPUS_RESULT_OK && ret != LAGOPUS_RESULT_NOT_OPERATIONAL) {
    lagopus_msg_warning("can't freed channels:%s.\n",
              lagopus_error_get_string(ret));
  }
  channel_list_free(arg);
}

static void
initialize_internal(void) {
  void *valptr = NULL;
  lagopus_result_t ret;
  lagopus_session_t s[2];
  struct channel_list *channel_free_list= NULL;

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

  channel_free_list = channel_list_alloc();
  if (channel_free_list == NULL) {
    lagopus_exit_fatal("channel_mgr_initialize:channel_list_alloc()");
  }

  /* Added channel free list. */
  valptr = channel_free_list;
  ret = lagopus_hashmap_add(&dp_table, (void *)UNUSED_DPID, (void **)&valptr, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("channel_mgr_initialize:lagopus_hashmap_add()");
  }

  ret = lagopus_callout_create_task(&channel_free_task, 0, "channel free task", 
                                        periodical_channel_free, NULL, NULL);

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("channel_mgr_initialize:lagopus_callout_create_task()");
  }

  ret = lagopus_callout_submit_task(&channel_free_task, 0, SEC_TO_NSEC(1));
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("channel_mgr_initialize:lagopus_callout_submit_task()");
  }

  ret = session_pair(SESSION_UNIX_DGRAM, s);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("channel_mgr_initialize:session_pair");
  }

  event_r = s[0];
  event_w = s[1];

  run = true;
}

void
channel_mgr_initialize(void) {
  lagopus_msg_debug(10, "called\n");
  pthread_once(&initialized, initialize_internal);
}

void
channel_mgr_finalize(void) {
  lagopus_result_t ret;

  lagopus_msg_debug(10, "called\n");
  run = false;
  lagopus_callout_cancel_task(&channel_free_task);
  channel_free_task = NULL;
  ret = lagopus_hashmap_clear(&main_table, true);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  ret = lagopus_hashmap_clear(&dp_table, true);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  session_destroy(event_r);
  session_destroy(event_w);
  event_r = NULL;
  event_w = NULL;

  return;
}

static lagopus_result_t
channel_delete_internal(const char *key) {
  uint64_t dpid;
  lagopus_result_t ret;
  struct channel *chan;
  struct channel_list *chan_list = NULL;

  ret = lagopus_hashmap_find(&main_table, (void *)key, (void **)&chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return LAGOPUS_RESULT_OK;
  }

  ret = lagopus_hashmap_delete(&main_table, (void *)key, NULL, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("can't delete channel(%p), %s \n",
                    chan, lagopus_error_get_string(ret));
  }

  channel_disable(chan);

  dpid = channel_dpid_get(chan);
  if (dpid != UNUSED_DPID) {
    ret = lagopus_hashmap_find(&dp_table, (void *)dpid, (void **)&chan_list);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("can't find channel(%p) in dp_table(%lu), %s \n",
                      chan, dpid, lagopus_error_get_string(ret));
    }

    ret = channel_list_delete(chan_list, chan);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("can't delete channel(%p) in channel_list(%lu), %s \n",
                      chan, dpid, lagopus_error_get_string(ret));
    }

    ret = channel_dpid_set(chan, UNUSED_DPID);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("can't set dpid to channel(%p), %s \n",
                                chan, lagopus_error_get_string(ret));
    }
  }

  /* Get free channel list. */
  ret = lagopus_hashmap_find(&dp_table, (void *)UNUSED_DPID, (void **)&chan_list);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("can't get free channel list:%s.", lagopus_error_get_string(ret));
  }

  ret = channel_list_insert(chan_list, chan);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("can't add channel to free channel list.");
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
channel_add_internal(const char *channel_name, lagopus_ip_address_t *addr,
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

  chan = channel_alloc(addr, UNUSED_DPID);
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
                                 uint64_t dpid, lagopus_ip_address_t *addr,
                                 struct channel **channel) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *chan = NULL;
  struct sockaddr *saddr = NULL;
  char *ipaddr = NULL;
  char key[MAIN_KEY_LEN];

  if ((ret = lagopus_ip_address_str_get(addr, &ipaddr)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = lagopus_ip_address_sockaddr_get(addr, &saddr)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  MAKE_MAIN_HASH(key, ipaddr, saddr, bridge_name);
  ret = channel_add_internal(key, addr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    goto done;
  }

  ret = channel_set_dpid(key, dpid);
  if (ret != LAGOPUS_RESULT_OK) {
    channel_mgr_channel_delete(bridge_name, addr);
  } else {
    *channel = chan;
  }

done:
  free(ipaddr);
  free(saddr);

  return ret;
}

lagopus_result_t
channel_mgr_channel_add(const char *bridge_name, uint64_t dpid,
                        lagopus_ip_address_t *addr) {
  lagopus_result_t ret;
  struct channel *chan = NULL;

  if (main_table == NULL) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }

  ret = lagopus_hashmap_size(&main_table);
  if (ret < 0) {
    return ret;
  }

  if (ret > MAX_CHANNELES) {
    return LAGOPUS_RESULT_TOO_MANY_OBJECTS;
  }

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
                            lagopus_ip_address_t *addr) {
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
channel_mgr_channel_lookup(const char *bridge_name,
                           lagopus_ip_address_t *addr,
                           struct channel **chan) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct sockaddr *saddr = NULL;
  char *ipaddr = NULL;
  char key[MAIN_KEY_LEN];

  if (main_table == NULL) {
    ret = LAGOPUS_RESULT_ANY_FAILURES;
    goto done;
  }

  if ((ret = lagopus_ip_address_str_get(addr, &ipaddr)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = lagopus_ip_address_sockaddr_get(addr, &saddr)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  MAKE_MAIN_HASH(key, ipaddr, saddr, bridge_name);
  if ((ret = lagopus_hashmap_find(&main_table, (void *)key, (void **)chan)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

done:
  free(ipaddr);
  free(saddr);

  return ret;
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
channel_mgr_channel_delete(const char *bridge_name, lagopus_ip_address_t *addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct sockaddr *saddr = NULL;
  char *ipaddr = NULL;
  char key[MAIN_KEY_LEN];

  if (main_table == NULL) {
    ret = LAGOPUS_RESULT_ANY_FAILURES;
    goto done;
  }

  if ((ret = lagopus_ip_address_str_get(addr, &ipaddr)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = lagopus_ip_address_sockaddr_get(addr, &saddr)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  MAKE_MAIN_HASH(key, ipaddr, saddr, bridge_name);
  if ((ret = channel_delete_internal((const char *) key)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

done:
  free(ipaddr);
  free(saddr);

  return ret;
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
  enum channel_protocol cprotocol;

  ret = channel_add_internal(channel_name, dst_addr, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = channel_port_set(chan, dst_port);
  if (ret != LAGOPUS_RESULT_OK) {
    goto fail;
  }

  if (src_addr != NULL) {
    ret = channel_local_addr_set(chan, src_addr);
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
  return channel_delete_internal(channel_name);
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
  lagopus_result_t ret;
  struct channel *chan;

  ret = channel_mgr_channel_lookup_by_name(channel_name, &chan);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }

  return channel_local_addr_get(chan, local_addr);
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

lagopus_result_t
channel_mgr_event_upcall(void) {
  bool w;
  ssize_t n;
  lagopus_result_t ret;
  lagopus_session_t s[1] = {event_w};
  const char *buf = "event up\n";

  lagopus_msg_debug(10, "called.\n");
  session_write_event_set(event_w);
  ret = session_poll(s, 1, 0);
  if (ret < 0) {
    lagopus_msg_warning("can't write upcall events, %s.\n",
                                lagopus_error_get_string(ret));
    return ret;
  }

  (void) session_is_writable(event_w, &w);
  if (w) {
    n = session_write(event_w, (void *) buf, strlen(buf) + 1);
    if (n < 0) {
      lagopus_msg_warning("event upcall failed. %d\n", (int) n);
      return LAGOPUS_RESULT_ANY_FAILURES;
    }
  } else {
    return LAGOPUS_RESULT_SOCKET_ERROR;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
channel_mgr_loop_stop(void) {
  run = false;
  channel_mgr_event_upcall();

  return LAGOPUS_RESULT_OK;
}
