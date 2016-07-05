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
 *      @file   tap_io.c
 *      @brief  Pseudo tap interface routines.
 */

#include <stdbool.h>
#include <inttypes.h>
#include <poll.h>
#include <err.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

#ifdef __linux__
#include <linux/if_tun.h>
#endif /* __linux__ */

#include "lagopus/dp_apis.h"
#include "lagopus/interface.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"
#include "tap_io.h"
#include "thread.h"

#define TUN_DEV "/dev/net/tun"

#define NUM_PORTID 256

struct dp_tap_interface {
  char *name;
  int fd;
  struct interface *ifp;
  uint32_t portid;
};

static struct pollfd pollfd[NUM_PORTID];
static struct dp_tap_interface *tap_ifp[NUM_PORTID];
static lagopus_hashmap_t tap_hashmap;
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

/**
 */
static int
tap_ioctl(struct dp_tap_interface *tap, int cmd, struct ifreq *ifreq) {
  strncpy(ifreq->ifr_name, tap->name, sizeof(ifreq->ifr_name));
  return ioctl(tap->fd, cmd, ifreq);
}

/**
 * Call SIOC[SG]* ioctl for tap interface.
 */
static lagopus_result_t
dp_tap_ifioctl(struct dp_tap_interface *tap, int cmd, struct ifreq *ifr) {
  int ctlfd;

  ctlfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (ctlfd == -1) {
    lagopus_msg_error("%s: socket: %s\n", __func__, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  snprintf(ifr->ifr_name, sizeof(ifr->ifr_name),
           "%s", tap->name);
  if (ioctl(ctlfd, cmd, ifr) < 0) {
    close(ctlfd);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  close(ctlfd);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_tap_interface_create(const char *name, struct interface *ifp) {
  struct ifreq ifr;
  struct dp_tap_interface *tap;
  char *nm;
  void *hashval;
  lagopus_result_t rv;

  tap = calloc(1, sizeof(struct dp_tap_interface));
  if (tap == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  nm = strrchr(name, ':');
  if (nm != NULL) {
    if (nm == name) {
      tap->name = strdup(nm + 1);
    } else {
      tap->name = strdup(name);
      nm = strrchr(tap->name, ':');
      *nm = INTERFACE_NAME_DELIMITER;
    }
  } else {
    tap->name = strdup(name);
  }
  if (tap->name == NULL) {
    free(tap);
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  tap->portid = get_port_number();
  if (tap->portid == UINT32_MAX) {
    free(tap);
    lagopus_msg_error("%s: too many port opened\n", name);
    return LAGOPUS_RESULT_TOO_MANY_OBJECTS;
  }
  if ((tap->fd = open(TUN_DEV, O_RDWR)) < 0) {
    free(tap);
    lagopus_msg_error("open(%s): %s", TUN_DEV, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  tap->ifp = ifp;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  if (tap_ioctl(tap, TUNSETIFF, &ifr) < 0) {
    free(tap->name);
    free(tap);
    lagopus_msg_error("tap_ioctl(%s, TUNSETIFF): %s",
                      ifr.ifr_name, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  memcpy(&ifr.ifr_hwaddr.sa_data, ifp->hw_addr, ETHER_ADDR_LEN);
  dp_tap_ifioctl(tap, SIOCSIFHWADDR, &ifr);
  hashval = tap;
  rv = lagopus_hashmap_add(&tap_hashmap, name, &hashval, false);
  if (rv == LAGOPUS_RESULT_OK) {
    tap_ifp[tap->portid] = tap;
    ifp->tap = tap;
    pollfd[tap->portid].fd = tap->fd;
  } else {
    close(tap->fd);
    free(tap->name);
    free(tap);
  }
  return rv;
}

dp_tap_interface_destroy(const char *name) {
  struct dp_tap_interface *tap;

  if (lagopus_hashmap_find(&tap_hashmap, name, &tap) == LAGOPUS_RESULT_OK) {
    lagopus_hashmap_delete(&tap_hashmap, name, NULL, false);
    pollfd[tap->portid].events = 0;
    pollfd[tap->portid].fd = 0;
    tap_ifp[tap->portid] = NULL;
    put_port_number(tap->portid);
    close(tap->fd);
    free(tap->name);
    free(tap);
    usleep(TAP_POLL_TIMEOUT * 1000);
  }
}

static lagopus_result_t
dp_tap_interface_link_set(struct dp_tap_interface *tap, bool updown) {
  struct ifreq ifr;

  if (dp_tap_ifioctl(tap, SIOCGIFFLAGS, &ifr) < 0) {
    lagopus_msg_error("%s: SIOCGIFFLAGS: %s (%d)\n",
                      tap->name, strerror(errno), errno);
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  if (updown) {
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
  } else  {
    ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
  }
  if (dp_tap_ifioctl(tap, SIOCSIFFLAGS, &ifr) < 0) {
    lagopus_msg_error("%s: SIOCSIFFLAGS: %s\n",
                      tap->name, strerror(errno));
    return LAGOPUS_RESULT_POSIX_API_ERROR;
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_tap_start_interface(const char *name) {
  struct ifreq ifr;
  struct dp_tap_interface *tap;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&tap_hashmap, name, &tap);
  if (rv == LAGOPUS_RESULT_OK) {
    dp_tap_interface_link_set(tap, true);
    pollfd[tap->portid].events = POLLIN;
  }
  return rv;
}

lagopus_result_t
dp_tap_stop_interface(const char *name) {
  struct ifreq ifr;
  struct dp_tap_interface *tap;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&tap_hashmap, name, &tap);
  if (rv == LAGOPUS_RESULT_OK) {
    dp_tap_interface_link_set(tap, false);
    pollfd[tap->portid].events = 0;
  }
  return rv;
}

/**
 * called from OFPP_NORMAL flow.
 */
lagopus_result_t
dp_tap_interface_send_packet(struct dp_tap_interface *tap,
                             struct lagopus_packet *pkt) {
  write(tap->fd, OS_MTOD(PKT2MBUF(pkt), uint8_t *), OS_M_PKTLEN(PKT2MBUF(pkt)));
  lagopus_packet_free(pkt);
  return LAGOPUS_RESULT_OK;
}

ssize_t
dp_tap_interface_recv_packet(struct dp_tap_interface *tap,
                             struct lagopus_packet *pkt) {
  return read(tap->fd, OS_MTOD(PKT2MBUF(pkt), uint8_t *), MAX_PACKET_SZ);
}

#ifdef HYBRID
static lagopus_thread_t tapio_thread = NULL;
static bool tapio_run = false;
static lagopus_mutex_t tapio_lock = NULL;

static lagopus_result_t
dp_tapio_thread_loop(__UNUSED const lagopus_thread_t *selfptr,
                     __UNUSED void *arg) {
  struct lagopus_packet *pkt;
  struct dp_tap_interface *tap;
  ssize_t len;
  int i;
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
  while (tapio_run == true) {
    switch (poll(pollfd, portidx, TAP_POLL_TIMEOUT)) {
      case -1:
        if (errno == EINTR) {
          continue;
        }
        err(errno, "poll");

      case 0:
        /* no events */
        continue;

      default:
        for (i = 0; i < portidx; i++) {
          if ((pollfd[i].revents & POLLIN) == 0) {
            continue;
          }
          tap = tap_ifp[i];
          pkt = alloc_lagopus_packet();
          if (pkt == NULL) {
            i--;
            continue;
          }
          (void)OS_M_APPEND(PKT2MBUF(pkt), MAX_PACKET_SZ);
          len = dp_tap_interface_recv_packet(tap, pkt);
          if (len < 0) {
            switch (errno) {
              case ENETDOWN:
              case ENETRESET:
              case ECONNABORTED:
              case ECONNRESET:
              case EINTR:
                continue;

              default:
                lagopus_exit_fatal("read: %d", errno);
            }
          }
          OS_M_TRIM(PKT2MBUF(pkt), MAX_PACKET_SZ - len);
          lagopus_packet_init(pkt, PKT2MBUF(pkt), tap->ifp->port);
          /* passthrough to real interface */
          lagopus_send_packet_physical(pkt, tap->ifp);
        }
        break;
    }
  }
}

lagopus_result_t
dp_tapio_thread_init(__UNUSED int argc,
                     __UNUSED const char *const argv[],
                     __UNUSED void *extarg,
                     lagopus_thread_t **thdptr) {
  static struct dataplane_arg dparg;

  lagopus_hashmap_create(&tap_hashmap, LAGOPUS_HASHMAP_TYPE_STRING, NULL);
  dparg.threadptr = &tapio_thread;
  dparg.lock = &tapio_lock;
  dparg.running = &tapio_run;
  lagopus_thread_create(&tapio_thread, dp_tapio_thread_loop,
                        dp_finalproc, dp_freeproc, "tap_io", &dparg);
  if (lagopus_mutex_create(&tapio_lock) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("lagopus_mutex_create");
  }
  *thdptr = &tapio_thread;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_tapio_thread_start(void) {
  return dp_thread_start(&tapio_thread, &tapio_lock, &tapio_run);
}

void
dp_tapio_thread_fini(void) {
  dp_thread_finalize(&tapio_thread);
  lagopus_hashmap_destroy(&tap_hashmap, true);
}

lagopus_result_t
dp_tapio_thread_shutdown(shutdown_grace_level_t level) {
  return dp_thread_shutdown(&tapio_thread, &tapio_lock, &tapio_run, level);
}

lagopus_result_t
dp_tapio_thread_stop(void) {
  return dp_thread_stop(&tapio_thread, &tapio_run);
}

uint32_t
dp_tapio_portid_get(const char *name) {
  struct dp_tap_interface *tap;
  if (lagopus_hashmap_find(&tap_hashmap, name, &tap) == LAGOPUS_RESULT_OK) {
    return tap->portid;
  }
  return 0;
}

lagopus_result_t
dp_tapio_interface_info_get(const char *in_name, uint8_t *hwaddr,
                            struct bridge **bridge) {
  struct dp_tap_interface *tap;
  char n[256] = ":";
  char *name = NULL;

  name = in_name;
  if (strchr(in_name, ':') == NULL) {
    sprintf(n, ":%s", in_name);
    name = n;
  }

  if (lagopus_hashmap_find(&tap_hashmap, name, &tap) == LAGOPUS_RESULT_OK) {
    memcpy(hwaddr, tap->ifp->hw_addr, ETHER_ADDR_LEN);
    *bridge = tap->ifp->port->bridge;
    return LAGOPUS_RESULT_OK;
  }

  return LAGOPUS_RESULT_NOT_FOUND;
}

struct interface*
dp_tapio_interface_get(const char *in_name) {
  struct dp_tap_interface *tap;
  char n[256] = ":";
  char *name = NULL;

  name = in_name;
  if (strchr(in_name, ':') == NULL) {
    sprintf(n, ":%s", in_name);
    name = n;
  }
  if (lagopus_hashmap_find(&tap_hashmap, name, &tap) == LAGOPUS_RESULT_OK) {
    return tap->ifp;
  }
  return NULL;
}

#if 0
#define MODIDX_TAPIO  LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 108

static void tapio_ctors(void) __attr_constructor__(MODIDX_TAPIO);
static void tapio_dtors(void) __attr_constructor__(MODIDX_TAPIO);

static void tapio_ctors (void) {
  lagopus_result_t rv;
  const char *name = "dp_tap_io";

  rv = lagopus_module_register(name,
                               dp_tapio_thread_init,
                               NULL,
                               dp_tapio_thread_start,
                               dp_tapio_thread_shutdown,
                               dp_tapio_thread_stop,
                               dp_tapio_thread_fini,
                               NULL);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
}
#endif /* 0 */
#endif /* HYBRID */
