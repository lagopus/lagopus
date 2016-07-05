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

/**
 *      @file   sock_io.c
 *      @brief  Dataplane driver use with raw socket
 */

#include "lagopus_config.h"

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <poll.h>
#include <pthread.h>
#include <err.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <net/if.h>
#include <pthread.h>

#include <linux/if_packet.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "lagopus/dp_apis.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofcache.h"
#include "lagopus/interface.h"
#include "pktbuf.h"
#include "packet.h"
#include "csum.h"
#include "thread.h"
#include "lock.h"
#include "sock_io.h"

#ifdef HAVE_DPDK
#include "dpdk.h"
#endif /* HAVE_DPDK */

static struct port_stats *rawsock_port_stats(struct port *port);

#define NUM_PORTID 256

static struct pollfd pollfd[NUM_PORTID];
static int ifindex[NUM_PORTID];

static bool no_cache = true;
static int kvs_type = FLOWCACHE_HASHMAP_NOLOCK;
static int hashtype = HASH_TYPE_INTEL64;
static struct flowcache *flowcache;

static bool volatile clear_cache = false;

static int portidx = 0;

static uint32_t free_portids[NUM_PORTID];

static uint32_t
get_port_number(void) {
  if (portidx == 0) {
    int i;

    /* initialize */
    for (i = 0; i < NUM_PORTID; i++) {
      free_portids[i] = i;
    }
  }
  if (portidx == NUM_PORTID) {
    /* portid exhausted */
    return UINT32_MAX;
  }
  return free_portids[portidx++];
}

static void
put_port_number(int id) {
  free_portids[--portidx] = id;
}

static ssize_t
read_packet(int fd, uint8_t *buf, size_t buflen) {
  struct sockaddr from;
  struct iovec iov;
  struct msghdr msg;
  union {
    struct cmsghdr cmsg;
    uint8_t buf[CMSG_SPACE(sizeof(struct tpacket_auxdata))];
  } cmsgbuf;
  struct cmsghdr *cmsg;
  struct tpacket_auxdata *auxdata;
  uint16_t *p;
  ssize_t pktlen;
  uint16_t ether_type;

  iov.iov_base = buf;
  iov.iov_len = buflen;
  memset(buf, 0, buflen); /* XXX */

  msg.msg_name = &from;
  msg.msg_namelen = sizeof(from);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = &cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);
  msg.msg_flags = 0;

  pktlen = recvmsg(fd, &msg, MSG_TRUNC);
  if (pktlen == -1) {
    if (errno == EAGAIN) {
      pktlen = 0;
    }
    return pktlen;
  }
  for (cmsg = CMSG_FIRSTHDR(&msg);
       cmsg != NULL;
       cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_type != PACKET_AUXDATA) {
      continue;
    }
    auxdata = (struct tpacket_auxdata *)CMSG_DATA(cmsg);
#if defined (TP_STATUS_VLAN_VALID)
    if ((auxdata->tp_status & TP_STATUS_VLAN_VALID) == 0) {
      continue;
    }
#else
    if (auxdata->tp_vlan_tci == 0) {
      continue;
    }
#endif /* TP_STATUS_VLAN_VALID */
    p = (uint16_t *)(buf + ETHER_ADDR_LEN * 2);
    switch (OS_NTOHS(p[0])) {
      case ETHERTYPE_PBB:
      case ETHERTYPE_VLAN:
        ether_type = 0x88a8;
        break;
      default:
        ether_type = ETHERTYPE_VLAN;
        break;
    }
    memmove(&p[2], p, pktlen - ETHER_ADDR_LEN * 2);
    p[0] = OS_HTONS(ether_type);
    p[1] = OS_HTONS(auxdata->tp_vlan_tci);
    pktlen += 4;
  }
  return pktlen;
}

lagopus_result_t
rawsock_rx_burst(struct interface *ifp, void *mbufs[], size_t nb) {
  lagopus_result_t i;

  for (i = 0; i < nb; i++) {
    struct lagopus_packet *pkt;
    ssize_t len;

    pkt = alloc_lagopus_packet();
    mbufs[i] = PKT2MBUF(pkt);
    len = read_packet(pollfd[ifp->info.eth_rawsock.port_number].fd,
                      OS_MTOD((OS_MBUF *)mbufs[i], uint8_t *), MAX_PACKET_SZ);
    if (len < 0) {
      switch (errno) {
        case ENETDOWN:
        case ENETRESET:
        case ECONNABORTED:
        case ECONNRESET:
        case EAGAIN:
          lagopus_packet_free(pkt);
          goto out;
        case EINTR:
          continue;

        default:
          lagopus_exit_fatal("read: %s", strerror(errno));
      }
    }
    if (len == 0) {
      lagopus_packet_free(pkt);
      break;
    }
    OS_M_TRIM((OS_MBUF *)mbufs[i], MAX_PACKET_SZ - len);
  }
out:
  return i;
}

#ifndef HAVE_DPDK
lagopus_result_t
rawsock_dataplane_init(int argc, const char *const argv[]) {
  static struct option lgopts[] = {
    {"no-cache", 0, 0, 0},
    {"kvstype", 1, 0, 0},
    {"hashtype", 1, 0, 0},
    {NULL, 0, 0, 0}
  };
  int opt, optind;

  while ((opt = getopt_long(argc, argv, "", lgopts, &optind)) != -1) {
    switch (opt) {
      case 0: /* long options */
        if (!strcmp(lgopts[optind].name, "no-cache")) {
          no_cache = true;
        }
        if (!strcmp(lgopts[optind].name, "kvstype")) {
          if (!strcmp(optarg, "hashmap_nolock")) {
            kvs_type = FLOWCACHE_HASHMAP_NOLOCK;
          } else if (!strcmp(optarg, "hashmap")) {
            kvs_type = FLOWCACHE_HASHMAP;
          } else {
            return -1;
          }
        }
        if (!strcmp(lgopts[optind].name, "hashtype")) {
          if (!strcmp(optarg, "intel64")) {
            hashtype = HASH_TYPE_INTEL64;
          } else if (!strcmp(optarg, "city64")) {
            hashtype = HASH_TYPE_CITY64;
          } else if (!strcmp(optarg, "murmur3")) {
            hashtype = HASH_TYPE_MURMUR3;
          } else {
            return -1;
          }
        }
        break;
    }
  }
  return 0;
}
#endif /* !HAVE_DPDK */

lagopus_result_t
rawsock_configure_interface(struct interface *ifp) {
  struct nlreq {
    struct nlmsghdr nlh;
    struct ifinfomsg ifinfo;
    char buf[64];
  } req;
  struct rtattr *rta;
  uint32_t portid;
  struct ifreq ifreq;
  struct packet_mreq mreq;
  struct sockaddr_ll sll;
  unsigned int mtu;
  int fd, on;

  fd = socket(PF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL));
  if (fd == -1) {
    lagopus_msg_error("%s: %s\n",
                      ifp->info.eth_rawsock.device, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  on = 1;
  if (setsockopt(fd, SOL_PACKET, PACKET_AUXDATA, &on, sizeof(on)) != 0) {
    close(fd);
    lagopus_msg_warning("%s: %s\n",
                        ifp->info.eth_rawsock.device, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name),
           "%s", ifp->info.eth_rawsock.device);
  if (ioctl(fd, SIOCGIFINDEX, &ifreq) != 0) {
    close(fd);
    lagopus_msg_warning("%s: %s\n",
                        ifp->info.eth_rawsock.device, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  portid = get_port_number();
  if (portid == UINT32_MAX) {
    close(fd);
    lagopus_msg_error("%s: too many port opened\n",
                      ifp->info.eth_rawsock.device);
    return LAGOPUS_RESULT_TOO_MANY_OBJECTS;
  }
  ifp->info.eth_rawsock.port_number = portid;
  ifindex[portid] = ifreq.ifr_ifindex;
  if (ioctl(fd, SIOCGIFHWADDR, &ifreq) != 0) {
    close(fd);
    lagopus_msg_warning("%s: %s\n",
                        ifp->info.eth_rawsock.device, strerror(errno));
  } else {
    memcpy(ifp->hw_addr, ifreq.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
  }
  lagopus_msg_info("Configuring %s, ifindex %d\n",
                   ifp->info.eth_rawsock.device, ifindex[portid]);
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex = ifindex[portid];
  bind(fd, (struct sockaddr *)&sll, sizeof(sll));

  /* Set MTU */
  memset(&req, 0, sizeof(req));
  req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.nlh.nlmsg_type = RTM_NEWLINK;
  req.nlh.nlmsg_flags = NLM_F_REQUEST;
  req.ifinfo.ifi_family = AF_UNSPEC;
  req.ifinfo.ifi_type = ARPHRD_ETHER;
  req.ifinfo.ifi_index = ifindex[portid];
  req.ifinfo.ifi_flags = 0;
  req.ifinfo.ifi_change = 0xffffffff;
  rta = (void *)(((char *)&req) + NLMSG_ALIGN(offsetof(struct nlreq, buf)));
  rta->rta_type = IFLA_MTU;
  rta->rta_len = RTA_LENGTH(sizeof(mtu));
  req.nlh.nlmsg_len = req.nlh.nlmsg_len + RTA_LENGTH(sizeof(mtu));
  mtu = ifp->info.eth_rawsock.mtu;
  memcpy(RTA_DATA(rta), &mtu, sizeof(mtu));
  send(fd, &req, req.nlh.nlmsg_len, 0);

  /* set promiscous mode */
  memset(&ifreq, 0, sizeof(ifreq));
  snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name),
           "%s", ifp->info.eth_rawsock.device);
  if (ioctl(fd, SIOCGIFFLAGS, &ifreq) != 0) {
    close(fd);
    lagopus_msg_warning("%s: %s\n",
                        ifp->info.eth_rawsock.device, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  ifreq.ifr_flags |= IFF_PROMISC;
  if (ioctl(fd, SIOCSIFFLAGS, &ifreq) != 0) {
    close(fd);
    lagopus_msg_warning("%s: %s\n",
                        ifp->info.eth_rawsock.device, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }

  pollfd[portid].fd = fd;
  pollfd[portid].events = 0;
  ifp->stats = rawsock_port_stats;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
rawsock_unconfigure_interface(struct interface *ifp) {
  uint32_t portid;

  portid = ifp->info.eth_rawsock.port_number;
  pollfd[portid].events = 0;
  close(pollfd[portid].fd);
  pollfd[portid].fd = 0;
  ifindex[portid] = 0;
  put_port_number(portid);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
rawsock_start_interface(struct interface *ifp) {
  uint32_t portid;

  portid = ifp->info.eth_rawsock.port_number;
  pollfd[portid].events = POLLIN;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
rawsock_stop_interface(struct interface *ifp) {
  uint32_t portid;

  portid = ifp->info.eth_rawsock.port_number;
  pollfd[portid].events = 0;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
rawsock_get_hwaddr(struct interface *ifp, uint8_t *hw_addr) {
  struct ifreq ifreq;
  uint32_t portid;
  int fd;

  portid = ifp->info.eth_rawsock.port_number;
  fd = pollfd[portid].fd;
  snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name),
           "%s", ifp->info.eth_rawsock.device);
  if (ioctl(fd, SIOCGIFHWADDR, &ifreq) != 0) {
    lagopus_msg_warning("%s\n", strerror(errno));
    return LAGOPUS_RESULT_ANY_FAILURES;
  }
  memcpy(hw_addr, ifreq.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
  return LAGOPUS_RESULT_OK;
}

int
rawsock_send_packet_physical(struct lagopus_packet *pkt, uint32_t portid) {
  if (pollfd[portid].fd != 0) {
    OS_MBUF *m;
    size_t plen;

    m = PKT2MBUF(pkt);
    plen = OS_M_PKTLEN(m);
    if (plen < 60) {
      memset(OS_M_APPEND(m, 60 - plen), 0, (uint32_t)(60 - plen));
    }
    if ((pkt->flags & PKT_FLAG_RECALC_CKSUM_MASK) != 0) {
      if (pkt->ether_type == ETHERTYPE_IP) {
        lagopus_update_ipv4_checksum(pkt);
      } else if (pkt->ether_type == ETHERTYPE_IPV6) {
        lagopus_update_ipv6_checksum(pkt);
      }
    }
    (void)write(pollfd[portid].fd,
                OS_MTOD(m, char *),
                OS_M_PKTLEN(m));
  }
  lagopus_packet_free(pkt);
  return 0;
}

static struct port_stats *
rawsock_port_stats(struct port *port) {
  struct {
    struct nlmsghdr nlh;
    struct ifinfomsg ifinfo;
    char buf[64];
  } req;
  union {
    struct nlmsghdr nlh;
    char buf[4096];
  } res;
  struct nlmsghdr *nlh;
  struct rtattr *rta;
#ifdef IFLA_STATS64
  struct rtnl_link_stats64 *link_stats;
#else
  struct rtnl_link_stats *link_stats;
#endif /* IFLA_STATS64 */
  struct timespec ts;
  struct port_stats *stats;
  int fd, len, rta_len;

  link_stats = NULL;

  stats = calloc(1, sizeof(struct port_stats));
  if (stats == NULL) {
    return NULL;
  }

  fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (fd == -1) {
    lagopus_msg_error("netlink socket create error: %s\n", strerror(errno));
    free(stats);
    return NULL;
  }
  memset(&req, 0, sizeof(req));
  req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.nlh.nlmsg_type = RTM_GETLINK;
  req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  req.ifinfo.ifi_family = AF_UNSPEC;
  req.ifinfo.ifi_type = ARPHRD_ETHER;
  req.ifinfo.ifi_index = ifindex[port->ifindex];
  req.ifinfo.ifi_flags = 0;
  req.ifinfo.ifi_change = 0xffffffff;
  send(fd, &req, req.nlh.nlmsg_len, 0);
  memset(&res, 0, sizeof(res));
  while ((len = recv(fd, &res, sizeof(res), 0)) > 0) {
    link_stats = NULL;
    for (nlh = &res.nlh; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
      if (nlh->nlmsg_type == RTM_NEWLINK) {
        struct ifinfomsg *ifi;

        ifi = NLMSG_DATA(nlh);
        if (ifi->ifi_index != ifindex[port->ifindex]) {
          continue;
        }
        if (ifi->ifi_family == AF_UNSPEC) {
          bool changed = false;

          if ((ifi->ifi_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
            if (port->ofp_port.state != OFPPS_LINK_DOWN) {
              lagopus_msg_info("Physical Port %d: link down\n", port->ifindex);
              changed = true;
            }
            port->ofp_port.state = OFPPS_LINK_DOWN;
          } else {
            if (port->ofp_port.state != OFPPS_LIVE) {
              lagopus_msg_info("Physical Port %d: link up\n", port->ifindex);
              changed = true;
            }
            port->ofp_port.state = OFPPS_LIVE;
          }
          if (changed == true) {
            send_port_status(port, OFPPR_MODIFY);
          }
          rta_len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));

          for (rta = IFLA_RTA(ifi);
               RTA_OK(rta, rta_len);
               rta = RTA_NEXT(rta, rta_len)) {

            switch (rta->rta_type) {
#ifdef IFLA_STATS64
              case IFLA_STATS64:
#else
              case IFLA_STATS:
#endif /* IFLA_STATS64 */
                link_stats = RTA_DATA(rta);
                goto found;
            }
          }
        }
      }
    }
  }
found:
  close(fd);

  /* if counter is not supported, set all ones value. */
  stats->ofp.port_no = port->ofp_port.port_no;
  if (link_stats != NULL) {
    stats->ofp.rx_packets = link_stats->rx_packets;
    stats->ofp.tx_packets = link_stats->tx_packets;
    stats->ofp.rx_bytes = link_stats->rx_bytes;
    stats->ofp.tx_bytes = link_stats->tx_bytes;
    stats->ofp.rx_dropped = link_stats->rx_dropped;
    stats->ofp.tx_dropped = link_stats->tx_dropped;
    stats->ofp.rx_errors = link_stats->rx_errors;
    stats->ofp.tx_errors = link_stats->tx_errors;
    stats->ofp.rx_frame_err = link_stats->rx_frame_errors;
    stats->ofp.rx_over_err = link_stats->rx_over_errors;
    stats->ofp.rx_crc_err =  link_stats->rx_crc_errors;
    stats->ofp.collisions = link_stats->collisions;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  stats->ofp.duration_sec = (uint32_t)(ts.tv_sec - port->create_time.tv_sec);
  if (ts.tv_nsec < port->create_time.tv_nsec) {
    stats->ofp.duration_sec--;
    stats->ofp.duration_nsec = 1 * 1000 * 1000 * 1000;
  } else {
    stats->ofp.duration_nsec = 0;
  }
  stats->ofp.duration_nsec += (uint32_t)ts.tv_nsec;
  stats->ofp.duration_nsec -= (uint32_t)port->create_time.tv_nsec;
  OS_MEMCPY(&port->ofp_port_stats, &stats->ofp, sizeof(stats->ofp));

  return stats;
}

lagopus_result_t
rawsock_get_stats(struct interface *ifp, datastore_interface_stats_t *stats) {
  struct {
    struct nlmsghdr nlh;
    struct ifinfomsg ifinfo;
    char buf[64];
  } req;
  union {
    struct nlmsghdr nlh;
    char buf[4096];
  } res;
  struct nlmsghdr *nlh;
  struct rtattr *rta;
#ifdef IFLA_STATS64
  struct rtnl_link_stats64 *link_stats;
#else
  struct rtnl_link_stats *link_stats;
#endif /* IFLA_STATS64 */
  ssize_t len, rta_len;
  int fd;

  link_stats = NULL;

  fd = socket(AF_NETLINK, SOCK_RAW | SOCK_NONBLOCK, NETLINK_ROUTE);
  if (fd == -1) {
    lagopus_msg_error("netlink socket create error: %s\n", strerror(errno));
    free(stats);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  memset(&req, 0, sizeof(req));
  req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.nlh.nlmsg_type = RTM_GETLINK;
  req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  req.ifinfo.ifi_family = AF_UNSPEC;
  req.ifinfo.ifi_type = ARPHRD_ETHER;
  req.ifinfo.ifi_index = ifindex[ifp->info.eth_rawsock.port_number];
  req.ifinfo.ifi_flags = 0;
  req.ifinfo.ifi_change = 0xffffffff;
  send(fd, &req, req.nlh.nlmsg_len, 0);
  memset(&res, 0, sizeof(res));
  while ((len = recv(fd, &res, sizeof(res), 0)) > 0) {
    link_stats = NULL;
    for (nlh = &res.nlh; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
      if (nlh->nlmsg_type == RTM_NEWLINK) {
        struct ifinfomsg *ifi;

        ifi = NLMSG_DATA(nlh);
        if (ifi->ifi_index != ifindex[ifp->info.eth_rawsock.port_number]) {
          continue;
        }
        if (ifi->ifi_family == AF_UNSPEC) {

          rta_len = nlh->nlmsg_len - NLMSG_HDRLEN - sizeof(struct ifinfomsg);
          for (rta = IFLA_RTA(ifi);
               RTA_OK(rta, rta_len);
               rta = RTA_NEXT(rta, rta_len)) {

            switch (rta->rta_type) {
#ifdef IFLA_STATS64
              case IFLA_STATS64:
#else
              case IFLA_STATS:
#endif /* IFLA_STATS64 */
                link_stats = RTA_DATA(rta);
                goto found;
            }
          }
        }
      }
    }
  }
found:
  close(fd);

  /* if counter is not supported, set all ones value. */
  if (link_stats != NULL) {
    stats->rx_packets = link_stats->rx_packets;
    stats->rx_bytes = link_stats->rx_bytes;
    stats->tx_packets = link_stats->tx_packets;
    stats->tx_bytes = link_stats->tx_bytes;
    stats->rx_errors = link_stats->rx_errors;
    stats->tx_dropped = link_stats->tx_dropped;
    stats->tx_errors = link_stats->tx_errors;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
rawsock_clear_stats(struct interface *ifp) {
  (void) ifp;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
rawsock_change_config(struct interface *ifp,
                      uint32_t advertised,
                      uint32_t config) {
  (void) ifp;
  (void) advertised;
  (void) config;

  return LAGOPUS_RESULT_OK;
}

void
clear_rawsock_flowcache(void) {
  clear_cache = true;
}

/**
 * Raw socket I/O process function.
 *
 * @param[in]   t       Thread object pointer.
 * @param[in]   arg     Do not used argument.
 */
static lagopus_result_t
dp_rawsock_thread_loop(__UNUSED const lagopus_thread_t *selfptr,
                    void *arg) {
  static const uint8_t eth_bcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  struct lagopus_packet *pkt;
  struct port_stats *stats;
  ssize_t len;
  unsigned int i;
  enum switch_mode mode;
  lagopus_result_t rv;
  global_state_t cur_state;
  shutdown_grace_level_t cur_grace;
  struct dataplane_arg *dparg;
  bool *running = NULL;

  rv = global_state_wait_for(GLOBAL_STATE_STARTED,
                             &cur_state,
                             &cur_grace,
                             -1);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  if (no_cache == false) {
    flowcache = init_flowcache(kvs_type);
  } else {
    flowcache = NULL;
  }

  dparg = arg;
  running = dparg->running;

  while (*running == true) {
    struct port *port;

    /* wait 0.1 sec. */
    if (poll(pollfd, (nfds_t)portidx, 100) < 0) {
      err(errno, "poll");
    }
    for (i = 0; i < portidx; i++) {
      if (clear_cache == true && flowcache != NULL) {
        clear_all_cache(flowcache);
        clear_cache = false;
      }
      flowdb_rdlock(NULL);
      port = dp_port_lookup(DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK,
                            (uint32_t)i);
      if (port == NULL) {
        flowdb_rdunlock(NULL);
        continue;
      }
      /* update port stats. */
      if (port->interface != NULL && port->interface->stats != NULL) {
        stats = port->interface->stats(port);
        free(stats);
      }

      if ((pollfd[i].revents & POLLIN) == 0) {
        flowdb_rdunlock(NULL);
        continue;
      }
      if (port->bridge != NULL &&
          (port->ofp_port.config & OFPPC_NO_RECV) == 0) {
        pkt = alloc_lagopus_packet();
        pkt->cache = flowcache;

        /* not enough? */
        (void)OS_M_APPEND(PKT2MBUF(pkt), MAX_PACKET_SZ);
        len = read_packet(pollfd[i].fd, OS_MTOD(PKT2MBUF(pkt), uint8_t *),
                          MAX_PACKET_SZ);
        if (len < 0) {
          switch (errno) {
            case ENETDOWN:
            case ENETRESET:
            case ECONNABORTED:
            case ECONNRESET:
            case EINTR:
              flowdb_rdunlock(NULL);
              continue;

            default:
              lagopus_exit_fatal("read: %s", strerror(errno));
          }
        }
        OS_M_TRIM(PKT2MBUF(pkt), MAX_PACKET_SZ - len);
        lagopus_packet_init(pkt, PKT2MBUF(pkt), port);
        flowdb_switch_mode_get(port->bridge->flowdb, &mode);
        if (
#ifdef HYBRID
                !memcmp(OS_MTOD(PKT2MBUF(pkt), uint8_t *),
                        port->interface->hw_addr, ETHER_ADDR_LEN) ||
                !memcmp(OS_MTOD(PKT2MBUF(pkt), uint8_t *),
                        eth_bcast, ETHER_ADDR_LEN) ||
#endif /* HYBRID */
                mode == SWITCH_MODE_STANDALONE) {
          lagopus_forward_packet_to_port(pkt, OFPP_NORMAL);
        } else {
          lagopus_match_and_action(pkt);
        }
      }
      flowdb_rdunlock(NULL);
    }
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_thread_t rawsock_thread = NULL;
static bool rawsock_run = false;
static lagopus_mutex_t rawsock_lock = NULL;

lagopus_result_t
dp_rawsock_thread_init(int argc,
                       const char *const argv[],
                       __UNUSED void *extarg,
                       lagopus_thread_t **thdptr) {
  static struct dataplane_arg dparg;
  lagopus_result_t nb_ports;

#ifdef HAVE_DPDK
  nb_ports = dpdk_dataplane_init(argc, argv);
#else
  nb_ports = rawsock_dataplane_init(argc, argv);
#endif /* HAVE_DPDK */
  if (nb_ports < 0) {
    lagopus_msg_fatal("lagopus_dataplane_init failed\n");
    return nb_ports;
  }
  lagopus_meter_init();
  lagopus_register_action_hook = lagopus_set_action_function;
  lagopus_register_instruction_hook = lagopus_set_instruction_function;
  flowinfo_init();

  dparg.threadptr = &rawsock_thread;
  dparg.lock = &rawsock_lock;
  dparg.running = &rawsock_run;
  lagopus_thread_create(&rawsock_thread, dp_rawsock_thread_loop,
                        dp_finalproc, dp_freeproc, "dp_rawsock",
                        &dparg);
  if (lagopus_mutex_create(&rawsock_lock) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("lagopus_mutex_create");
  }
  *thdptr = &rawsock_thread;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_rawsock_thread_start(void) {
#ifdef HAVE_DPDK
  no_cache = app.no_cache;
  kvs_type = app.kvs_type;
  hashtype = app.hashtype;
#endif /* HAVE_DPDK */
  return dp_thread_start(&rawsock_thread, &rawsock_lock, &rawsock_run);
}

lagopus_result_t
dp_rawsock_thread_stop(void) {
  return dp_thread_stop(&rawsock_thread, &rawsock_run);
}

lagopus_result_t
dp_rawsock_thread_shutdown(shutdown_grace_level_t level) {
  return dp_thread_shutdown(&rawsock_thread, &rawsock_lock, &rawsock_run,
                            level);
}

void
dp_rawsock_thread_fini(void) {
  dp_thread_finalize(&rawsock_thread);
}

#if 0
#define MODIDX_RAWSOCK  LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 108
#define MODNAME_RAWSOCK "dp_rawsock"

static void rawsock_ctors(void) __attr_constructor__(MODIDX_RAWSOCK);
static void rawsock_dtors(void) __attr_constructor__(MODIDX_RAWSOCK);

static void rawsock_ctors (void) {
  lagopus_result_t rv;
  const char *name = "dp_rawsock";

  dp_api_init();
  rv = lagopus_module_register(name,
                               dp_rawsock_thread_init,
                               NULL,
                               dp_rawsock_thread_start,
                               dp_rawsock_thread_shutdown,
                               dp_rawsock_thread_stop,
                               dp_rawsock_thread_fini,
                               NULL);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
}

static void rawsock_dtors (void) {
  dp_api_fini();
}
#endif /* 0 */
