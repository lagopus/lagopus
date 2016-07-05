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

#define TAP_POLL_TIMEOUT 1

struct dp_tap_interface;

lagopus_result_t
dp_tap_interface_create(const char *name, struct interface *ifp);
dp_tap_interface_destroy(const char *name);
lagopus_result_t
dp_tap_start_interface(const char *name);
lagopus_result_t
dp_tap_stop_interface(const char *name);
lagopus_result_t
dp_tap_interface_send_packet(struct dp_tap_interface *tap,
                             struct lagopus_packet *pkt);
ssize_t
dp_tap_interface_recv_packet(struct dp_tap_interface *tap,
                             struct lagopus_packet *pkt);
lagopus_result_t
dp_tap_thread_loop(__UNUSED const lagopus_thread_t *selfptr,
                   __UNUSED void *arg);
uint32_t
dp_tapio_portid_get(const char *name);
struct interface*
dp_tapio_interface_get(const char *name);
lagopus_result_t
dp_tapio_interface_info_get(const char *in_name, uint8_t *hwaddr,
                            struct bridge **bridge);
