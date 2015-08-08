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

/**
 *      @file   bpf_io.c
 *      @brief  Datapath driver use with Berkelay Packet Filter
 */

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include "lagopus_apis.h"
#include "lagopus/interface.h"

/* XXX not implemented yet */

lagopus_result_t
rawsock_configure_interface(struct interface *ifp) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

lagopus_result_t
rawsock_unconfigure_interface(struct interface *ifp) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

lagopus_result_t
rawsock_start_interface(struct interface *ifp) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

lagopus_result_t
rawsock_stop_interface(struct interface *ifp) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

lagopus_result_t
rawsock_get_hwaddr(struct interface *ifp, uint8_t *hw_addr) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

int
rawsock_send_packet_physical(struct lagopus_packet *pkt, uint32_t portid) {
  return 0;
}

lagopus_result_t
rawsock_get_stats(struct interface *ifp, datastore_interface_stats_t *stats) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

lagopus_result_t
rawsock_clear_stats(struct interface *ifp) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

lagopus_result_t
rawsock_change_config(struct interface *ifp,
                      uint32_t advertised,
                      uint32_t config) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

/**
 * dataplane thread (raw socket I/O)
 */
lagopus_result_t
rawsocket_thread_loop(__UNUSED const lagopus_thread_t *selfptr, void *arg) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}
