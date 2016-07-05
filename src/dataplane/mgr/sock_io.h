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

#ifndef SRC_DATAPLANE_MGR_SOCK_IO_H_
#define SRC_DATAPLANE_MGR_SOCK_IO_H_

#define SOCK_POLL_TIMEOUT 100 /* msec */

ssize_t
dp_rawsock_interface_recv_packet(int fd, uint8_t *buf, size_t buflen);

lagopus_result_t
rawsock_rx_burst(struct interface *ifp, void *mbufs[], size_t nb);

  void clear_rawsock_flowcache(void);

#endif /* SRC_DATAPLANE_MGR_SOCK_IO_H_ */
