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


/* netlink interface to the kernel. */

#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include "netlink.h"

int
netlink_socket_create(struct netlink *netlink) {
  int sock;
  int ret;
  struct sockaddr_nl remote;
  struct sockaddr_nl local;
  socklen_t local_len;

  netlink->sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
  if (netlink->sock < 0) {
    return -1;
  }

  memset(&remote, 0, sizeof(remote));
  remote.nl_family = AF_NETLINK;
  ret = connect(netlink->sock, (struct sockaddr *)&remote, sizeof(remote));
  if (ret < 0) {
    goto error;
  }

  memset(&local, 0, sizeof(local));
  local_len = sizeof(local);
  ret = getsockname(netlink->sock, (struct sockaddr *)&local, &local_len);
  if (ret < 0) {
    goto error;
  }
  netlink->pid = local.nl_pid;

error:
  if (ret < 0) {
    netlink_socket_close(netlink);
    return ret;
  }

  return sock;
}

void
netlink_socket_close(struct netlink *netlink) {
  if (netlink->sock > 0) {
    close(netlink->sock);
  }
}

int
netlink_socket_send(struct netlink *netlink) {
  int ret;
}
