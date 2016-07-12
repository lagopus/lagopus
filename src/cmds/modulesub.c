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
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/dataplane.h"
#include "lagopus/dp_apis.h"
#include "lagopus/ofp_handler.h"
#ifdef ENABLE_SNMP_MODULE
#include "lagopus/snmpmgr.h"
#endif /* ENABLE_SNMP_MODULE */
#include "agent.h"
#include "ofp_dpqueue_mgr.h"
#include "lagopus/datastore.h"





static lagopus_result_t
agent_initialize_wrap(int argc, const char *const argv[],
                      void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;

  return agent_initialize(NULL, retptr);
}


static inline void
agent_finalize_wrap(void) {
  agent_finalize();
}

static lagopus_result_t
ofp_handler_initialize_wrap(int argc, const char *const argv[],
                            void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;
  (void)retptr;

  return ofp_handler_initialize(NULL, NULL);
}


static inline void
ofp_handler_finalize_wrap(void) {
  ofp_handler_finalize();
}


#ifdef ENABLE_SNMP_MODULE
static lagopus_result_t
snmpmgr_initialize_wrap(int argc, const char *const argv[],
                        void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;

  return snmpmgr_initialize((void *)true, retptr);
}
#endif /* ENABLE_SNMP_MODULE */





#define CTOR_IDX	LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 3


static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static void	s_ctors(void) __attr_constructor__(CTOR_IDX);
static void	s_dtors(void) __attr_destructor__(CTOR_IDX);


static void
s_once_proc(void) {
  lagopus_result_t r;
  const char *name;

  name = "datastore";
  if ((r = lagopus_module_register(name,
                                   datastore_initialize, NULL,
                                   datastore_start,
                                   datastore_shutdown,
                                   datastore_stop,
                                   datastore_finalize,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

  dp_api_init();
  name = "dp_rawsock";
  r = lagopus_module_register(name,
                              dp_rawsock_thread_init,
                              NULL,
                              dp_rawsock_thread_start,
                              dp_rawsock_thread_shutdown,
                              dp_rawsock_thread_stop,
                              dp_rawsock_thread_fini,
                              NULL);
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
#ifdef HAVE_DPDK
  name = "dp_dpdk";
  r = lagopus_module_register(name,
                              dp_dpdk_thread_init,
                              NULL,
                              dp_dpdk_thread_start,
                              dp_dpdk_thread_shutdown,
                              dp_dpdk_thread_stop,
                              dp_dpdk_thread_fini,
                              dp_dpdk_thread_usage);
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
#endif /* HAVE_DPDK */
  name = "dp_timer";
  r = lagopus_module_register(name,
                              dp_timer_thread_init,
                              NULL,
                              dp_timer_thread_start,
                              dp_timer_thread_shutdown,
                              dp_timer_thread_stop,
                              dp_timer_thread_fini,
                              NULL);
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
  name = "dpqueuemgr";
  r = lagopus_module_register(name,
                              ofp_dpqueue_mgr_initialize,
                              NULL,
                              ofp_dpqueue_mgr_start,
                              ofp_dpqueue_mgr_shutdown,
                              ofp_dpqueue_mgr_stop,
                              ofp_dpqueue_mgr_finalize,
                              NULL);
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
#ifdef HYBRID
  name = "dp_tap_io";
  r = lagopus_module_register(name,
                              dp_tapio_thread_init,
                              NULL,
                              dp_tapio_thread_start,
                              dp_tapio_thread_shutdown,
                              dp_tapio_thread_stop,
                              dp_tapio_thread_fini,
                              NULL);
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

  name = "dp_netlink";
  r = lagopus_module_register(name,
                              dp_netlink_thread_init,
                              NULL,
                              dp_netlink_thread_start,
                              dp_netlink_thread_shutdown,
                              dp_netlink_thread_stop,
                              dp_netlink_thread_fini,
                              NULL);
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

#ifdef PIPELINER
  name = "dp_legacy_sw_pipeline";
  r = lagopus_module_register(name,
                              dp_legacy_sw_thread_init,
                              NULL,
                              dp_legacy_sw_thread_start,
                              dp_legacy_sw_thread_shutdown,
                              dp_legacy_sw_thread_stop,
                              dp_legacy_sw_thread_fini,
                              NULL);
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
#endif /* PIPELINER */
#endif /* HYBRID */

  name = "agent";
  if ((r = lagopus_module_register(name,
                                   agent_initialize_wrap, NULL,
                                   agent_start,
                                   agent_shutdown,
                                   agent_stop,
                                   agent_finalize_wrap,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

  name = "ofp_handler";
  if ((r = lagopus_module_register(name,
                                   ofp_handler_initialize_wrap, NULL,
                                   ofp_handler_start,
                                   ofp_handler_shutdown,
                                   ofp_handler_stop,
                                   ofp_handler_finalize_wrap,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

#ifdef ENABLE_SNMP_MODULE
  name = "snmpmgr";
  if ((r = lagopus_module_register(name,
                                   snmpmgr_initialize_wrap, NULL,
                                   snmpmgr_start,
                                   snmpmgr_shutdown,
                                   snmpmgr_stop,
                                   snmpmgr_finalize,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
#endif /* ENABLE_SNMP_MODULE */

  name = "load_conf";
  if ((r = lagopus_module_register(name,
                                   load_conf_initialize, NULL,
                                   load_conf_start,
                                   load_conf_shutdown,
                                   NULL,
                                   load_conf_finalize,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(5, "called.\n");
}


static inline void
s_final(void) {
}


static void
s_dtors(void) {
  s_final();

  lagopus_msg_debug(5, "called.\n");
}
