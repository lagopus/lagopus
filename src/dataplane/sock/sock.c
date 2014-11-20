/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
 *      @file   sock.c
 *      @brief  Datapath driver use with raw socket
 */

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
#include <net/ethernet.h>
#include <net/if.h>
#include <pthread.h>

#include <linux/if_packet.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofcache.h"
#include "pktbuf.h"
#include "packet.h"
#include "pcap.h"

static struct port_stats *port_stats(struct port *port);

static struct pollfd pollfd[256]; /* XXX */
static int ifindex[256];

#define TIMEOUT_SHUTDOWN_RIGHT_NOW     (100*1000*1000) /* 100msec */
#define TIMEOUT_SHUTDOWN_GRACEFULLY    (1500*1000*1000) /* 1.5sec */

static bool no_cache = true;
static int kvs_type = FLOWCACHE_HASHMAP_NOLOCK;
static int hashtype = HASH_TYPE_INTEL64;
static bool volatile clear_cache = false;

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
    if (auxdata->tp_vlan_tci == 0) {
      continue;
    }
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
datapath_thread_loop(__UNUSED const lagopus_thread_t *, __UNUSED void *);

struct lagopus_packet *
alloc_lagopus_packet(void) {
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m) + sizeof(*pkt));
  if (m == NULL) {
    lagopus_msg_error("mbuf alloc failed\n");
    return NULL;
  }
  pkt = (struct lagopus_packet *)&m[1];
  m->data = &m->dat[128];
  pkt->mbuf = m;

  return pkt;
}

void
sock_m_free(OS_MBUF *m) {
  if (m->refcnt-- <= 0) {
    free(m);
  }
}

lagopus_result_t
lagopus_configure_physical_port(struct port *port) {
  uint32_t portid;
  struct ifreq ifreq;
  struct packet_mreq mreq;
  struct sockaddr_ll sll;
  int fd, on;

  fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (fd == -1) {
    lagopus_msg_warning("%s: %s\n", port->ofp_port.name, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  on = 1;
  if (setsockopt(fd, SOL_PACKET, PACKET_AUXDATA, &on, sizeof(on)) != 0) {
    lagopus_msg_warning("%s: %s\n", port->ofp_port.name, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name), "%s", port->ofp_port.name);
  if (ioctl(fd, SIOCGIFINDEX, &ifreq) != 0) {
    close(fd);
    lagopus_msg_warning("%s: %s\n", port->ofp_port.name, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  portid = port->ifindex;
  ifindex[portid] = ifreq.ifr_ifindex;
  if (ioctl(fd, SIOCGIFHWADDR, &ifreq) != 0) {
    close(fd);
    lagopus_msg_warning("%s: %s\n", port->ofp_port.name, strerror(errno));
  }
  memcpy(port->ofp_port.hw_addr, ifreq.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
  lagopus_msg_info("Configuring %s, ifindex %d\n",
                   port->ofp_port.name, ifindex[port->ifindex]);
  memset(&mreq, 0, sizeof(mreq));
  mreq.mr_type = PACKET_MR_PROMISC;
  mreq.mr_ifindex = ifindex[port->ifindex];
  setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
             (void *)&mreq, sizeof(mreq));
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex = ifindex[port->ifindex];
  bind(fd, (struct sockaddr *)&sll, sizeof(sll));
  pollfd[portid].fd = fd;
  pollfd[portid].events = POLLIN;
  port->stats = port_stats;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
lagopus_unconfigure_physical_port(struct port *port) {
  uint32_t portid;

  portid = port->ifindex;
  close(pollfd[portid].fd);
  pollfd[portid].fd = 0;
  ifindex[portid] = 0;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
lagopus_change_physical_port(struct port *port) {

  send_port_status(port, OFPPR_MODIFY);
  return LAGOPUS_RESULT_OK;
}

bool
lagopus_is_port_enabled(__UNUSED const struct port *port) {
  return true;
}

void
lagopus_packet_free(struct lagopus_packet *pkt) {
  OS_M_FREE(pkt->mbuf);
}

int
lagopus_send_packet_physical(struct lagopus_packet *pkt, uint32_t portid) {
  if (pollfd[portid].fd != 0) {
    (void)write(pollfd[portid].fd, pkt->mbuf->data, OS_M_PKTLEN(pkt->mbuf));
  }
  lagopus_packet_free(pkt);
  return 0;
}

int
lagopus_send_packet_normal(__UNUSED struct lagopus_packet *pkt,
                           __UNUSED uint32_t portid) {
  /* not implemented yet */
  return 0;
}

struct lagopus_packet *
copy_packet(struct lagopus_packet *src_pkt) {
  return src_pkt;
}

void
lagopus_instruction_experimenter(__UNUSED struct lagopus_packet *pkt,
                                 __UNUSED uint32_t exp_id) {
  /* writing your own instruction */
}

int
lagopus_meter_packet(__UNUSED struct lagopus_packet *pkt,
                     __UNUSED struct meter *meter,
                     __UNUSED uint8_t *prec_level) {
  /* XXX not implemented */
  return 0;
}

void
lagopus_meter_init(void) {
  /* XXX not implemented */
}

lagopus_result_t
lagopus_datapath_init(int argc, const char *argv[]) {
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
          } else if (!strcmp(optarg, "ptree")) {
            kvs_type = FLOWCACHE_PTREE;
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

void
clear_worker_flowcache(bool wait_flush) {

  (void) wait_flush;

  if (no_cache == true) {
    return;
  }
  clear_cache = true;
}

static struct port_stats *
port_stats(struct port *port) {
  struct sockaddr_nl sa;
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
    nlh = &res;
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

/**
 * datapath thread
 */
lagopus_result_t
datapath_thread_loop(__UNUSED const lagopus_thread_t *selfptr, void *arg) {
  struct datapath_arg *dparg;
  struct lagopus_packet pkt;
  struct dpmgr *dpmgr;
  struct sock_buf *m;
  struct port_stats *stats;
  ssize_t len;
  unsigned int nb_ports;
  unsigned int i;
  lagopus_result_t rv;
  global_state_t cur_state;
  shutdown_grace_level_t cur_grace;

  rv = global_state_wait_for(GLOBAL_STATE_STARTED,
                             &cur_state,
                             &cur_grace,
                             -1);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  dparg = arg;
  dpmgr = dparg->dpmgr;

  if (no_cache == false) {
    pkt.cache = init_flowcache(kvs_type);
  } else {
    pkt.cache = NULL;
  }

  for (;;) {
    struct port *port;
    struct flowdb *flowdb;

    nb_ports = vector_max(dpmgr->ports) + 1;

    /* wait 0.1 sec. */
    if (poll(pollfd, (nfds_t)nb_ports, 100) < 0) {
      err(errno, "poll");
    }
    for (i = 0; i < nb_ports; i++) {
      if (clear_cache == true && pkt.cache != NULL) {
        clear_all_cache(pkt.cache);
        clear_cache = false;
      }
      port = port_lookup(dpmgr->ports, (uint32_t)i);
      if (port == NULL) {
        continue;
      }
      /* update port stats. */
      stats = port->stats(port);
      free(stats);

      if ((pollfd[i].revents & POLLIN) == 0) {
        continue;
      }
      if (port->bridge != NULL &&
          (port->ofp_port.config & OFPPC_NO_RECV) == 0) {
        m = calloc(1, sizeof(*m));
        if (m == NULL) {
          lagopus_exit_fatal("calloc: %d", errno);
        }
        m->data = &m->dat[128];
        /* not enough? */
        len = read_packet(pollfd[i].fd, m->data, 1518);
        if (len < 0 && errno != EINTR) {
          lagopus_exit_fatal("read: %d", errno);
        }
        m->len = (size_t)len;

        flowdb = port->bridge->flowdb;
        flowdb_rdlock(flowdb);
        lagopus_set_in_port(&pkt, port);
        lagopus_packet_init(&pkt, m);
        if (port->bridge->flowdb->switch_mode == SWITCH_MODE_STANDALONE) {
          pkt.in_port = port;
          lagopus_forward_packet_to_port(&pkt, OFPP_NORMAL);
        } else {
          lagopus_receive_packet(port, &pkt);
        }
        flowdb_rdunlock(flowdb);
      }
    }
  }

  return LAGOPUS_RESULT_OK;
}

void
get_flowcache_statistics(struct bridge *bridge, struct ofcachestat *st) {
  st->nentries = 0;
  st->hit = 0;
  st->miss = 0;
  /* not implemented yet */
}

void
datapath_usage(__UNUSED FILE *fp) {
  /* so far, nothing additional options. */
}
