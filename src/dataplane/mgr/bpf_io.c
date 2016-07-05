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
 *      @file   bpf_io.c
 *      @brief  Datapath driver use with Berkelay Packet Filter
 */

#include "lagopus_config.h"

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>

#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofcache.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/interface.h"
#include "pktbuf.h"
#include "packet.h"
#include "csum.h"
#include "thread.h"
#include "lock.h"

static struct port_stats *bpf_port_stats(struct port *port);

#define BPF_DEV "/dev/bpf"
#define NUM_PORTID 256
#define BPF_PACKET_BUFSIZE 65536

static struct pollfd pollfd[NUM_PORTID];
struct pkt_buffers {
  uint8_t *buf;
  uint8_t *ptr;
  ssize_t len;
};
static struct pkt_buffers pkt_buffers[NUM_PORTID];

#ifndef HAVE_DPDK
static bool no_cache = true;
static int kvs_type = FLOWCACHE_HASHMAP_NOLOCK;
static int hashtype = HASH_TYPE_INTEL64;
static struct flowcache *flowcache;
#endif /* HAVE_DPDK */

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
read_packet(int portid, uint8_t *buf, size_t buflen) {
  struct pkt_buffers *pbp;
  struct bpf_hdr *hdr;
  ssize_t pktlen;

  pbp = &pkt_buffers[portid];
  if (pbp->len == 0) {
    pbp->len = read(pollfd[portid].fd, pbp->buf, BPF_PACKET_BUFSIZE);
    if (pbp->len == -1) {
      if (errno == EAGAIN) {
        pbp->len = 0;
      }
      return pbp->len;
    }
    pbp->ptr = pbp->buf;
  }
  if (pbp->buf + pbp->len <= pbp->ptr) {
    pbp->len = 0;
    return 0;
  }
  hdr = (void *)pbp->ptr;
  pktlen = hdr->bh_caplen;
  OS_MEMCPY(buf, pbp->ptr + hdr->bh_hdrlen, hdr->bh_caplen);
  pbp->ptr += hdr->bh_hdrlen + hdr->bh_caplen;
  return pktlen;
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
  uint32_t portid;
  struct ifreq ifreq;
  u_int intparam;
  int fd, on;

  fd = open(BPF_DEV, O_RDWR);
  if (fd == -1) {
    lagopus_msg_error("%s: %s\n",
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
  pollfd[portid].fd = fd;
  intparam = BPF_PACKET_BUFSIZE;
  if (ioctl(fd, BIOCSBLEN, &intparam) != 0) {
    lagopus_msg_error("%s: BIOCSBLEN: %s\n",
                      ifp->info.eth_rawsock.device, strerror(errno));
    close(fd);
    put_port_number(portid);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  pkt_buffers[portid].buf = malloc(BPF_PACKET_BUFSIZE);
  if (pkt_buffers[portid].buf == NULL) {
    lagopus_msg_error("%s: malloc: %s\n",
                      ifp->info.eth_rawsock.device, strerror(errno));
    close(fd);
    put_port_number(portid);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  pkt_buffers[portid].len = 0;
#ifdef BIOCSDIRECTION
  intparam = BPF_D_IN;
  if (ioctl(fd, BIOCSDIRECTION, &intparam) != 0) {
    lagopus_msg_error("%s: BIOCSDIRECTION: %s\n",
                      ifp->info.eth_rawsock.device, strerror(errno));
    close(fd);
    put_port_number(portid);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
#else
  intparam = 0;
  if (ioctl(fd, BIOCSSEESENT, &intparam) != 0) {
    lagopus_msg_error("%s: BIOCSSEESENT: %s\n",
                      ifp->info.eth_rawsock.device, strerror(errno));
    close(fd);
    put_port_number(portid);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
#endif /* BIOCSDIRECTION */
  snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name), "%s",
           ifp->info.eth_rawsock.device);
  if (ioctl(fd, BIOCSETIF, &ifreq) != 0) {
    lagopus_msg_error("%s: BIOCSETIF: %s\n",
                      ifp->info.eth_rawsock.device, strerror(errno));
    close(fd);
    put_port_number(portid);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  lagopus_msg_info("Configuring %s\n", ifp->info.eth_rawsock.device);
  ifreq.ifr_addr.sa_family = AF_LINK;
  if (ioctl(fd, SIOCGIFADDR, &ifreq) == 0) {
    memcpy(ifp->hw_addr,
           LLADDR(((struct sockaddr_dl *)&ifreq.ifr_addr)),
           ETHER_ADDR_LEN);
  }
  if (ioctl(fd, BIOCPROMISC, NULL) != 0) {
    lagopus_msg_error("%s: BIOCPROMISC: %s\n",
                      ifp->info.eth_rawsock.device, strerror(errno));
  }
  on = 1;
#ifdef BIOCIMMEDIATE
  if (ioctl(fd, BIOCIMMEDIATE, &on) == 0) {
  }
#endif /* BIOCIMMEDIATE */
  pollfd[portid].events = 0;
  ifp->stats = bpf_port_stats;

  return LAGOPUS_RESULT_OK;

}

lagopus_result_t
rawsock_unconfigure_interface(struct interface *ifp) {
  uint32_t portid;

  portid = ifp->info.eth_rawsock.port_number;
  pollfd[portid].events = 0;
  close(pollfd[portid].fd);
  pollfd[portid].fd = 0;
  free(pkt_buffers[portid].buf);
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
  ifreq.ifr_addr.sa_family = AF_LINK;
  if (ioctl(fd, SIOCGIFADDR, &ifreq) != 0) {
    lagopus_msg_warning("%s\n", strerror(errno));
    return LAGOPUS_RESULT_ANY_FAILURES;
  }
  memcpy(ifp->hw_addr,
      LLADDR(((struct sockaddr_dl *)&ifreq.ifr_addr)),
      ETHER_ADDR_LEN);
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
bpf_port_stats(struct port *port) {
  struct port_stats *stats;

  stats = calloc(1, sizeof(struct port_stats));
  if (stats == NULL) {
    return NULL;
  }

  return stats;
}

lagopus_result_t
rawsock_get_stats(struct interface *ifp, datastore_interface_stats_t *stats) {
  (void) ifp;

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

/**
 * Raw socket I/O process function.
 *
 * @param[in]   t       Thread object pointer.
 * @param[in]   arg     Do not used argument.
 */
static lagopus_result_t
dp_bpf_thread_loop(__UNUSED const lagopus_thread_t *selfptr, void *arg) {
  static const uint8_t eth_bcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  struct lagopus_packet *pkt;
  struct port_stats *stats;
  ssize_t len;
  unsigned int i;
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

#ifndef HAVE_DPDK
  if (no_cache == false) {
    flowcache = init_flowcache(kvs_type);
  } else {
    flowcache = NULL;
  }
#endif /* HAVE_DPDK */

  dparg = arg;
  running = dparg->running;

  while (*running == true) {
    struct port *port;

    /* wait 0.1 sec. */
    if (poll(pollfd, (nfds_t)portidx, 100) < 0) {
      err(errno, "poll");
    }
    for (i = 0; i < portidx; i++) {
#ifndef HAVE_DPDK
      if (clear_cache == true && flowcache != NULL) {
        clear_all_cache(flowcache);
        clear_cache = false;
      }
#endif /* HAVE_DPDK */
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
        for (;;) {
          enum switch_mode switch_mode;

          pkt = alloc_lagopus_packet();
#ifndef HAVE_DPDK
          if (flowcache != NULL) {
            pkt->cache = flowcache;
          }
#endif /* HAVE_DPDK */
        /* not enough? */
          (void)OS_M_APPEND(PKT2MBUF(pkt), MAX_PACKET_SZ);
          len = read_packet(i, OS_MTOD(PKT2MBUF(pkt), uint8_t *), MAX_PACKET_SZ);
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
                lagopus_exit_fatal("read_packet(%d, %p, %d): %s",
                                   pollfd[i].fd, OS_MTOD(PKT2MBUF(pkt), uint8_t *),
                                   MAX_PACKET_SZ, strerror(errno));
            }
          } else if (len == 0) {
            /*
             * read all packet in buffer.
             * stop for loop and look in next port
             */
            lagopus_packet_free(pkt);
            break;
          }
          OS_M_TRIM(PKT2MBUF(pkt), MAX_PACKET_SZ - len);
          lagopus_packet_init(pkt, PKT2MBUF(pkt), port);
          flowdb_switch_mode_get(port->bridge->flowdb, &switch_mode);
          if (
#ifdef HYBRID
              !memcmp(OS_MTOD(PKT2MBUF(pkt), uint8_t *),
                      port->interface->hw_addr, ETHER_ADDR_LEN) ||
              !memcmp(OS_MTOD(PKT2MBUF(pkt), uint8_t *),
                      eth_bcast, ETHER_ADDR_LEN) ||
#endif /* HYBRID */
              switch_mode == SWITCH_MODE_STANDALONE) {
            lagopus_forward_packet_to_port(pkt, OFPP_NORMAL);
          } else {
            lagopus_match_and_action(pkt);
          }
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
  lagopus_thread_create(&rawsock_thread, dp_bpf_thread_loop,
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
