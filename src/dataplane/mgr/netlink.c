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
 *      @file   netlink.c
 *      @brief  Linux netlink interface.
 */

#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <errno.h>

#include "lagopus/dp_apis.h"
#include "lagopus/interface.h"
#include "lagopus/dataplane.h"
#include "netlink.h"
#include "event.h"
#include "rib_notifier.h"
#include "thread.h"

#ifdef HYBRID
#define INTERFACE_NAME_MAX 512
#endif /* HYBRID */

/* Prototypes. */
const char *nlmsg_str(__u16 nlmsg_type);
void rtattr_parse(struct rtattr **tb, int max, struct rtattr *rta, int len);
int netlink_start(struct event_manager *em);

/* For missing NDA_RTA macro. */
#ifndef NDA_RTA
#define NDA_RTA(r)  ((struct rtattr*)(((char*)(r)) +                    \
                                      NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif

/* Netlink socket structure. */
struct nlsock {
  const char *name;
  int sock;
  __u32 seq;
  struct sockaddr_nl snl;
};

/* Netlink monitor and command structure. */
struct nlsock netlink_monitor = {
  .name = "netlink-monitor",
  .sock = -1,
  .seq = 0,
  .snl = { 0 }
};
struct nlsock netlink_command = {
  .name = "netlink-command",
  .sock = -1,
  .seq = 0,
  .snl = { 0 }
};

const char *
nlmsg_str(__u16 nlmsg_type) {
  switch (nlmsg_type) {
    case RTM_NEWLINK:
      return "RTM_NEWLINK";
    case RTM_DELLINK:
      return "RTM_DELLINK";
    case RTM_GETLINK:
      return "RTM_GETLINK";
    case RTM_SETLINK:
      return "RTM_SETLINK";
    case RTM_NEWADDR:
      return "RTM_NEWADDR";
    case RTM_DELADDR:
      return "RTM_DELADDR";
    case RTM_GETADDR:
      return "RTM_GETADDR";
    case RTM_NEWROUTE:
      return "RTM_NEWROUTE";
    case RTM_DELROUTE:
      return "RTM_DELROUTE";
    case RTM_GETROUTE:
      return "RTM_GETROUTE";
    case RTM_NEWNEIGH:
      return "RTM_NEWNEIGH";
    case RTM_DELNEIGH:
      return "RTM_DELNEIGH";
    case RTM_GETNEIGH:
      return "RTM_GETNEIGH";
    case RTM_NEWRULE:
      return "RTM_NEWRULE";
    case RTM_DELRULE:
      return "RTM_DELRULE";
    case RTM_GETRULE:
      return "RTM_GETRULE";
    default:
      return "Unknown nlmsg_type";
  }
}

void
rtattr_parse(struct rtattr **tb, int max, struct rtattr *rta, int len) {
  while (RTA_OK(rta, len)) {
    if (rta->rta_type <= max) {
      tb[rta->rta_type] = rta;
    }
    rta = RTA_NEXT(rta, len);
  }
}

static int
netlink_read(struct nlsock *nlsock,
             int (*filter)(struct sockaddr_nl *, struct nlmsghdr *)) {
  int rc = 0;
  ssize_t status;
  int error;

  while (1) {
    char buf[8192];
    struct iovec iov = {
      .iov_base = buf,
      .iov_len = sizeof buf
    };
    struct sockaddr_nl snl;
    struct msghdr msg = {
      .msg_name = (void *)&snl,
      .msg_namelen = sizeof snl,
      .msg_iov = &iov,
      .msg_iovlen = 1
    };
    struct nlmsghdr *h;

    /* Receive netlink message from kernel. */
    status = recvmsg(nlsock->sock, &msg, 0);

    /* In case of EINTR try again.  In case of EWOULDBLOCK or EAGAIN,
       return to the caller and expect to be called again. */
    if (status < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        break;
      }
      lagopus_msg_error("%s recvmsg overrun: %s", nlsock->name,
                        strerror(errno));
      continue;
    }

    /* End of the message. */
    if (status == 0) {
      lagopus_msg_error("%s EOF", nlsock->name);
      return -1;
    }

    /* Name length check. */
    if (msg.msg_namelen != sizeof snl) {
      lagopus_msg_error("%s sender address length error: length %d",
                        nlsock->name, msg.msg_namelen);
      return -1;
    }

    for (h = (struct nlmsghdr *)buf; NLMSG_OK(h, (unsigned int)status);
         h = NLMSG_NEXT(h, status)) {

      /* Finish of reading. */
      if (h->nlmsg_type == NLMSG_DONE) {
        return rc;
      }

      /* Error handling. */
      if (h->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
        int errnum = err->error;
        __u16 msg_type = err->msg.nlmsg_type;

        /* If the error field is zero, then this is an ACK. */
        if (err->error == 0) {
          lagopus_msg_error("%s: %s ACK: %s(%u), seq=%u, pid=%u\n",
                            __FUNCTION__, nlsock->name,
                            nlmsg_str(err->msg.nlmsg_type),
                            err->msg.nlmsg_type,
                            err->msg.nlmsg_seq,
                            err->msg.nlmsg_pid);

          /* return if not a multipart message, otherwise continue */
          if (!(h->nlmsg_flags & NLM_F_MULTI)) {
            return 0;
          }
          continue;
        }

        if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
          lagopus_msg_error("%s error: message truncated\n", nlsock->name);
          return -1;
        }

        /* Deal with errors that occur because of races in link
           handling. */
        if (nlsock == &netlink_command
            && ((msg_type == RTM_DELROUTE &&
                 (-errnum == ENODEV || -errnum == ESRCH))
                || (msg_type == RTM_NEWROUTE && -errnum == EEXIST))) {
          lagopus_msg_error("%s: error: %s %s(%u), seq=%u, pid=%u\n",
                            nlsock->name, strerror(-errnum),
                            nlmsg_str(msg_type),
                            msg_type, err->msg.nlmsg_seq, err->msg.nlmsg_pid);
          return 0;
        }

        lagopus_msg_error("%s error: %s, %s(%u), seq=%u, pid=%u",
                          nlsock->name, strerror(-errnum),
                          nlmsg_str(msg_type),
                          msg_type, err->msg.nlmsg_seq, err->msg.nlmsg_pid);
        return -1;
      }

      /* OK we got netlink message. */
#ifdef NETLINK_DEBUG
      printf("netlink_read: %s %s(%u), seq=%u, pid=%u\n",
             nlsock->name, nlmsg_str(h->nlmsg_type), h->nlmsg_type,
             h->nlmsg_seq, h->nlmsg_pid);
#endif

      /* Skip unsolicited messages originating from command socket
         linux sets the originators port-id for NEWADDR or DELADDR
         messages, so this has to be checked here. */
      if (nlsock != &netlink_command &&
          h->nlmsg_pid == netlink_command.snl.nl_pid &&
          (h->nlmsg_type != RTM_NEWADDR && h->nlmsg_type != RTM_DELADDR)) {
        lagopus_msg_error("netlink_read: %s packet comes from %s",
                          netlink_command.name, nlsock->name);
        continue;
      }

      error = (*filter)(&snl, h);
      if (error < 0) {
        lagopus_msg_error("%s filter function error", nlsock->name);
        rc = error;
      }
    }

    /* Message is truncated. */
    if (msg.msg_flags & MSG_TRUNC) {
      lagopus_msg_error("%s error: message truncated", nlsock->name);
      continue;
    }

    /* If status is non zero, something was wrong. */
    if (status) {
      lagopus_msg_error("%s error: data remnant size %lu", nlsock->name,
                        status);
      return -1;
    }
  }
  return rc;
}

static int
netlink_send(struct nlsock *nlsock, unsigned char family, __u16 type) {
  ssize_t rc;
  struct sockaddr_nl snl;
  struct {
    struct nlmsghdr nlh;
    struct rtgenmsg g;
  } req;

  if (nlsock->sock < 0) {
    return -1;
  }

  memset(&snl, 0, sizeof snl);
  memset(&req, 0, sizeof req);

  snl.nl_family = AF_NETLINK;

  req.nlh.nlmsg_len = sizeof req;
  req.nlh.nlmsg_type = type;
  req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
  req.nlh.nlmsg_pid = nlsock->snl.nl_pid;
  req.nlh.nlmsg_seq = ++nlsock->seq;
  req.g.rtgen_family = family;

  rc = sendto(nlsock->sock, (void *)&req, sizeof req, 0,
              (struct sockaddr *)&snl, sizeof snl);
  if (rc < 0) {
    printf("%s sendto failed: %s", nlsock->name, strerror(errno));
    return -1;
  }
  return 0;
}

static int
netlink_addr(__UNUSED struct sockaddr_nl *snl, struct nlmsghdr *h) {
  long unsigned int len;
  struct ifaddrmsg *ifa;
  struct rtattr *tb[IFA_MAX + 1] = { 0 };
  int ifindex;
  void *addr;
  void *broad;
  char *label = NULL;
#ifdef HYBRID
  char *nm = NULL;
  char ifname[INTERFACE_NAME_MAX + 1] = {0};
#endif /* HYBRID */

  if (NLMSG_LENGTH(sizeof(struct ifaddrmsg))> h->nlmsg_len) {
    return -1;
  }

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifaddrmsg));

  ifa = (struct ifaddrmsg *)NLMSG_DATA(h);

  if (ifa->ifa_family != AF_INET && ifa->ifa_family != AF_INET6) {
    return 0;
  }

  rtattr_parse(tb, IFA_MAX, IFA_RTA(ifa), len);

  ifindex = (int)ifa->ifa_index;

  /* This logic copied from iproute2/ip/ipaddress.c:print_addrinfo(). */
  if (tb[IFA_LOCAL] == NULL) {
    tb[IFA_LOCAL] = tb[IFA_ADDRESS];
  }
  if (tb[IFA_ADDRESS] == NULL) {
    tb[IFA_ADDRESS] = tb[IFA_LOCAL];
  }

  if (!tb[IFA_LOCAL]) {
    return 0;
  }

  addr = RTA_DATA(tb[IFA_LOCAL]);

  if (tb[IFA_ADDRESS] &&
      memcmp(RTA_DATA(tb[IFA_ADDRESS]), RTA_DATA(tb[IFA_LOCAL]),
             RTA_PAYLOAD(tb[IFA_ADDRESS]))) {
    broad = RTA_DATA(tb[IFA_ADDRESS]);
  } else {
    broad = (tb[IFA_BROADCAST] ? RTA_DATA(tb[IFA_BROADCAST]) : NULL);
  }

  if (ifa->ifa_flags & IFA_F_SECONDARY) {
    /* TODO: secondary flag should be handled for IPv4. */
  }

  if (tb[IFA_LABEL]) {
    label =(char *) RTA_DATA(tb[IFA_LABEL]);
  }

#ifdef HYBRID
  if (label) {
    /* set ip address to interface object */
    nm = strrchr(label, INTERFACE_NAME_DELIMITER );
    if (nm != NULL) {
      *nm = ':';
    }
    if (ifa->ifa_family == AF_INET) {
      dp_interface_ip_set(label, AF_INET, addr, broad, ifa->ifa_prefixlen);
    } else {
      dp_interface_ip_set(label, AF_INET6, addr, broad, ifa->ifa_prefixlen);
    }
  }
#endif /* HYBRID */

  if (ifa->ifa_family == AF_INET) {
    if (h->nlmsg_type == RTM_NEWADDR) {
      rib_notifier_ipv4_addr_add(ifindex, (struct in_addr *)addr,
                                 ifa->ifa_prefixlen,
                                 (struct in_addr *)broad, label);
    } else {
#ifdef HYBRID
      dp_interface_ip_unset(label);
#endif
      rib_notifier_ipv4_addr_delete(ifindex, (struct in_addr *)addr,
                                    ifa->ifa_prefixlen,
                                    (struct in_addr *)broad, label);
    }
  } else {
    if (h->nlmsg_type == RTM_NEWADDR) {
      rib_notifier_ipv6_addr_add(ifindex, (struct in6_addr *)addr,
                                 ifa->ifa_prefixlen,
                                 (struct in6_addr *)broad, label);
    } else {
      rib_notifier_ipv6_addr_delete(ifindex, (struct in6_addr *)addr,
                                    ifa->ifa_prefixlen,
                                    (struct in6_addr *)broad, label);
    }
  }

  return 0;
}

static void
masklen2ip(const int masklen, struct in_addr *netmask) {
  if (sizeof(unsigned long long) > 4) {
    netmask->s_addr = htonl(0xffffffffULL << (32 - masklen));
  } else {
    netmask->s_addr = htonl(masklen ? 0xffffffffU << (32 - masklen) : 0);
  }
}

static void
apply_mask_ipv4(struct in_addr *addr, int prefixlen) {
  struct in_addr mask;

  masklen2ip(prefixlen, &mask);
  addr->s_addr &= mask.s_addr;
}

static int
netlink_route(__UNUSED struct sockaddr_nl *snl, struct nlmsghdr *h) {
  long unsigned int len;
  struct rtmsg *rtm;
  struct rtattr *tb[RTA_MAX + 1] = { 0 };
  void *dest;
  void *gate = NULL;
  int plen;
  char anyaddr[16] = { 0 };
  int ifindex = 0;

  if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct rtmsg))) {
    return -1;
  }

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof(struct rtmsg));

  rtm = (struct rtmsg *)NLMSG_DATA(h);

  if (rtm->rtm_family != AF_INET && rtm->rtm_family != AF_INET6) {
    return 0;
  }

  if (rtm->rtm_type != RTN_UNICAST) {
    return 0;
  }

  if (rtm->rtm_flags & RTM_F_CLONED) {
    return 0;
  }

  if (rtm->rtm_protocol == RTPROT_REDIRECT) {
    return 0;
  }

  if (rtm->rtm_src_len != 0) {
    return 0;
  }

  rtattr_parse(tb, RTA_MAX, RTM_RTA(rtm), len);

  if (tb[RTA_DST]) {
    dest = RTA_DATA(tb[RTA_DST]);
  } else {
    dest = anyaddr;
  }

  plen = rtm->rtm_dst_len;

  if (tb[RTA_OIF]) {
    ifindex = *(int *)RTA_DATA(tb[RTA_OIF]);
  }

  if (tb[RTA_GATEWAY]) {
    gate = RTA_DATA(tb[RTA_GATEWAY]);
  }

  if (rtm->rtm_family == AF_INET) {
    struct in_addr p;
    struct in_addr g;

    memcpy(&p, dest, 4);
    apply_mask_ipv4(&p, plen);

    if (gate) {
      memcpy(&g, gate, 4);
    } else {
      memset(&g, 0, 4);
    }

    if (h->nlmsg_type == RTM_NEWROUTE) {
      rib_notifier_ipv4_route_add(&p, plen, &g, ifindex, rtm->rtm_scope);
    } else {
      rib_notifier_ipv4_route_delete(&p, plen, &g, ifindex);
    }
  }  else {
    struct in6_addr p;
    struct in6_addr g;

    memcpy(&p, dest, 16);
    if (gate) {
      memcpy(&g, gate, 16);
    }

    if (h->nlmsg_type == RTM_NEWROUTE) {
      rib_notifier_ipv6_route_add((struct in6_addr *)dest, plen,
                                  &g, ifindex);
    } else {
      rib_notifier_ipv6_route_delete((struct in6_addr *)dest, plen,
                                  &g, ifindex);
    }
  }
  return 0;
}

static void
netlink_ifparam_parse(struct ifparam_t *param, struct rtattr **tb,
                      struct ifinfomsg *ifi) {
  param->ifindex = ifi->ifi_index;
  param->flags = ifi->ifi_flags & 0x0000fffff;
  param->mtu = *(int *)RTA_DATA(tb[IFLA_MTU]);

  if (tb[IFLA_IFNAME]) {
    strncpy(param->name, (char *)RTA_DATA(tb[IFLA_IFNAME]), IFNAMSIZ);
  }

  if (tb[IFLA_ADDRESS]) {
    int hw_addr_len = RTA_PAYLOAD(tb[IFLA_ADDRESS]);
    memcpy(param->hw_addr, RTA_DATA(tb[IFLA_ADDRESS]), hw_addr_len);
    param->hw_addr_len = hw_addr_len;
  }
}

static int
netlink_link(__UNUSED struct sockaddr_nl *snl, struct nlmsghdr *h) {
  long unsigned int len;
  struct ifinfomsg *ifi;
  struct rtattr *tb[IFLA_MAX + 1] = { 0 };
  struct ifparam_t param = { 0 };
  int ifindex;

  if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct ifinfomsg))) {
    return -1;
  }

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifinfomsg));

  ifi = (struct ifinfomsg *)NLMSG_DATA(h);

  rtattr_parse(tb, IFLA_MAX, IFLA_RTA(ifi), len);

  if ((tb[IFLA_WIRELESS] != NULL) && (ifi->ifi_change == 0)) {
    return 0;
  }

  if (tb[IFLA_IFNAME] == NULL) {
    return -1;
  }

  ifindex = ifi->ifi_index;

  if (h->nlmsg_type == RTM_NEWLINK) {
    netlink_ifparam_parse(&param, tb, ifi);
    rib_notifier_interface_update(ifindex, &param);
  } else {
    rib_notifier_interface_delete(ifindex);
  }

  return 0;
}

static int
netlink_neigh(__UNUSED struct sockaddr_nl *snl, struct nlmsghdr *h) {
  long unsigned int len;
  struct ndmsg *ndm;
  struct rtattr *tb[NDA_MAX + 1] = { 0 };
  void *dst_addr = NULL;
  char *ll_addr = NULL;

  if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct ndmsg))) {
    return -1;
  }

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof(struct ndmsg));

  ndm = (struct ndmsg *)NLMSG_DATA(h);

  if (ndm->ndm_family != AF_INET && ndm->ndm_family != AF_INET6) {
    return 0;
  }

#ifdef NETLINK_DEBUG
  printf("ndm_state (0x%02x) ifindex %d: ", ndm->ndm_state, ndm->ndm_ifindex);
  if (ndm->ndm_state & NUD_INCOMPLETE) {
    printf("incomplete ");
  }
  if (ndm->ndm_state & NUD_REACHABLE) {
    printf("reachable ");
  }
  if (ndm->ndm_state & NUD_STALE) {
    printf("stale ");
  }
  if (ndm->ndm_state & NUD_DELAY) {
    printf("delay ");
  }
  if (ndm->ndm_state & NUD_PROBE) {
    printf("probe ");
  }
  if (ndm->ndm_state & NUD_FAILED) {
    printf("failed ");
  }
  if (ndm->ndm_state & NUD_NOARP) {
    printf("no-arp ");
  }
  if (ndm->ndm_state & NUD_PERMANENT) {
    printf("permanent ");
  }
  printf("\n");
#endif /* NETLINK_DEBUG */

  if (ndm->ndm_state & NUD_NOARP) {
    return 0;
  }

  rtattr_parse(tb, NDA_MAX, NDA_RTA(ndm), len);

  if (tb[NDA_DST]) {
    dst_addr = RTA_DATA(tb[NDA_DST]);
  }

  if (tb[NDA_LLADDR]) {
    ll_addr = RTA_DATA(tb[NDA_LLADDR]);
  }

  if (ndm->ndm_family == AF_INET) {
    if (h->nlmsg_type == RTM_NEWNEIGH && ll_addr != NULL) {
      rib_notifier_arp_add(ndm->ndm_ifindex,
                           (struct in_addr *)dst_addr, ll_addr);
    } else {
      rib_notifier_arp_delete(ndm->ndm_ifindex,
                              (struct in_addr *)dst_addr, ll_addr);
    }
  } else {
    if (h->nlmsg_type == RTM_NEWNEIGH) {
      rib_notifier_ndp_add(ndm->ndm_ifindex,
                           (struct in6_addr *)dst_addr, ll_addr);
    } else {
      rib_notifier_ndp_delete(ndm->ndm_ifindex,
                              (struct in6_addr *)dst_addr, ll_addr);
    }
  }
  return 0;
}

static int
netlink_fetch(struct sockaddr_nl *snl, struct nlmsghdr *h) {
  /* Ignore messages that aren't from the kernel. */
  if (snl->nl_pid != 0) {
    printf("Ignoring message from pid %u", snl->nl_pid);
    return 0;
  }

  switch (h->nlmsg_type) {
    case RTM_NEWLINK:
      return netlink_link(snl, h);
    case RTM_DELLINK:
      return netlink_link(snl, h);
    case RTM_NEWADDR:
      return netlink_addr(snl, h);
    case RTM_DELADDR:
      return netlink_addr(snl, h);
    case RTM_NEWROUTE:
      return netlink_route(snl, h);
    case RTM_DELROUTE:
      return netlink_route(snl, h);
    case RTM_NEWNEIGH:
      return netlink_neigh(snl, h);
    case RTM_DELNEIGH:
      return netlink_neigh(snl, h);
    default:
      printf("Unknown netlink nlmsg_type %d\n", h->nlmsg_type);
      return 0;
  }
}

static void
netlink_read_event(struct event *event) {
  struct event_manager *em;

  em = event_get_manager(event);
  netlink_read(&netlink_monitor, netlink_fetch);
  event_register_read(em, netlink_monitor.sock, netlink_read_event, NULL);
}

static int
netlink_dump(struct nlsock *nlsock) {
  int rc = 0;

  struct {
    unsigned char family;
    __u16 type;
    int (*filter)(struct sockaddr_nl *, struct nlmsghdr *);
  } cmds[] = {
    { AF_PACKET, RTM_GETLINK,  netlink_link },
    { AF_INET,   RTM_GETNEIGH, netlink_neigh },
    { AF_INET6,  RTM_GETNEIGH, netlink_neigh },
    { AF_INET,   RTM_GETADDR,  netlink_addr },
    { AF_INET6,  RTM_GETADDR,  netlink_addr },
    { AF_INET,   RTM_GETROUTE, netlink_route },
    { AF_INET6,  RTM_GETROUTE, netlink_route },
    { 0, 0, NULL },
  };

  for (int i = 0; cmds[i].family != 0; i++) {
    rc = netlink_send(nlsock, cmds[i].family, cmds[i].type);
    if (rc < 0) {
      return rc;
    }
    rc = netlink_read(nlsock, cmds[i].filter);
    if (rc < 0) {
      return rc;
    }
  }

  return rc;
}

static void
netlink_recv_bufsize_set(int sock) {
  int rc;
  const int bufsiz = (20 * 1024 * 1024);

  /* Linux >= 2.6.14 has force option.  Try it first. */
  rc = setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE, &bufsiz, sizeof bufsiz);
  if (rc < 0) {
    /* Max socket recvbuf value is /proc/sys/net/core/rmem_max. */
    rc = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsiz, sizeof bufsiz);
    if (rc < 0) {
      /* printf("setsockopt error %s\n", strerror(errno)); */
    }
  }
}

static int
netlink_open(struct nlsock *nlsock, __u32 groups) {
  int rc;
  int sock;
  socklen_t snlen;

  /* Socket address for netlink. */
  struct sockaddr_nl snl = {
    .nl_family = AF_NETLINK,
    .nl_pad = 0,
    .nl_pid = 0,
    .nl_groups = groups
  };

  /* Socket for netlink. */
  sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0) {
    printf("Can't open nlsock\n");
    return sock;
  }

  /* Set receive buffer size. */
  netlink_recv_bufsize_set(sock);

  /* Bind the socket. */
  snlen = sizeof snl;
  rc = bind(sock, (struct sockaddr *)&snl, snlen);
  if (rc < 0) {
    printf("Can't bind socket %s\n", strerror(errno));
    close(sock);
    return rc;
  }

  /* Call getsockname() for getting pid. */
  rc = getsockname(sock, (struct sockaddr *)&snl, &snlen);
  if (rc < 0 || snlen != sizeof snl) {
    printf("Can't getsockname()\n");
    close(sock);
    return rc;
  }

  /* Copy value back to nlsock. */
  nlsock->sock = sock;
  nlsock->snl = snl;

  return rc;
}

int
netlink_start(struct event_manager *em) {
  __u32 groups = (RTMGRP_LINK|RTMGRP_NEIGH|
                  RTMGRP_IPV4_ROUTE|RTMGRP_IPV4_IFADDR|
                  RTMGRP_IPV6_ROUTE|RTMGRP_IPV6_IFADDR);

  netlink_open(&netlink_monitor, groups);
  netlink_open(&netlink_command, 0);

  if (netlink_command.sock >= 0) {
    netlink_dump(&netlink_command);
  }

  if (netlink_monitor.sock >= 0) {
    event_register_read(em, netlink_monitor.sock, netlink_read_event, NULL);
  }

  return 0;
}

#ifdef HYBRID
static lagopus_thread_t netlink_thread = NULL;
static bool netlink_run = false;
static lagopus_mutex_t netlink_lock = NULL;

static lagopus_result_t
dp_netlink_thread_loop(__UNUSED const lagopus_thread_t *selfptr,
                       __UNUSED void *arg) {
  struct event_manager *em;
  struct event *event;

  rib_notifier_init();

  em = event_manager_alloc();
  if (em == NULL) {
    lagopus_msg_error("dp_netlink_thread_loop(): event_manager alloc failed\n");
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  netlink_start(em);

  while ((event = event_fetch(em)) != NULL) {
    event_call(event);
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_netlink_thread_init(__UNUSED int argc,
                       __UNUSED const char *const argv[],
                       __UNUSED void *extarg,
                       lagopus_thread_t **thdptr) {
  static struct dataplane_arg dparg;

  dparg.threadptr = &netlink_thread;
  dparg.lock = &netlink_lock;
  dparg.running = &netlink_run;
  lagopus_thread_create(&netlink_thread, dp_netlink_thread_loop,
                        dp_finalproc, dp_freeproc, "netlink", &dparg);
  if (lagopus_mutex_create(&netlink_lock) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("lagopus_mutex_create");
  }
  *thdptr = &netlink_thread;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_netlink_thread_start(void) {
  return dp_thread_start(&netlink_thread, &netlink_lock, &netlink_run);
}

void
dp_netlink_thread_fini(void) {
  dp_thread_finalize(&netlink_thread);
}

lagopus_result_t
dp_netlink_thread_shutdown(shutdown_grace_level_t level) {
  return dp_thread_shutdown(&netlink_thread, &netlink_lock,
                            &netlink_run, level);
}

lagopus_result_t
dp_netlink_thread_stop(void) {
  return dp_thread_stop(&netlink_thread, &netlink_run);
}
#endif /* HYBRID */

