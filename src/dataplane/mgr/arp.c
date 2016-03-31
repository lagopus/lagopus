/* %COPYRIGHT% */

/**
 *      @file   arp.h
 *      @brief  ARP table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lagopus/dp_apis.h"
#include <net/ethernet.h>
#include "arp.h"

#undef ARP_DEBUG
#ifdef ARP_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static lagopus_hashmap_t arp_entry_hashmap;
lagopus_rwlock_t arp_lock;
int cstate;

struct arp_entry {
  int ifindex;
  char mac_addr[ETHER_ADDR_LEN];
};

#define IPV4_BITLEN   32
#define IPV4_SIZE      4
#define IPV6_BITLEN  128
#define IPV6_SIZE     16

/* static functions */
static lagopus_result_t
arp_entry_free(struct arp_entry *entry) {
  if (entry == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  free(entry);
  return LAGOPUS_RESULT_OK;
}

static void
arp_log(const char *type_str, int ifindex, struct in_addr *dst_addr,
            char *ll_addr) {
  if (dst_addr) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET, dst_addr, buf, BUFSIZ);

    PRINTF("ARP %s: %s ->", type_str, buf);

    if (ll_addr) {
      PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x",
             ll_addr[0] & 0xff, ll_addr[1] & 0xff, ll_addr[2] & 0xff,
             ll_addr[3] & 0xff, ll_addr[4] & 0xff, ll_addr[5] & 0xff);
    }

    PRINTF(" ifindex: %u", ifindex);

    PRINTF("\n");
  }
}

static lagopus_result_t
arp_delete_entry(int ifindex, struct in_addr *dst_addr, char *ll_addr)
{
  lagopus_result_t rv;
  struct arp_entry *entry;
  char gw[BUFSIZ];

  inet_ntop(AF_INET, dst_addr, gw, BUFSIZ);
  lagopus_rwlock_writer_enter_critical(&arp_lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&arp_entry_hashmap,
                            (void *)gw, (void **)&entry);

  if (entry != NULL || rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_delete_no_lock(&arp_entry_hashmap,
                                 (void *)gw, (void **)&entry, true);
    if (rv != LAGOPUS_RESULT_OK) {
      (void)lagopus_rwlock_leave_critical(&arp_lock, cstate);
      return rv;
    }
  }
  (void)lagopus_rwlock_leave_critical(&arp_lock, cstate);

  return rv;
}

static lagopus_result_t
arp_update_entry(int ifindex, struct in_addr *dst_addr, char *ll_addr)
{
  lagopus_result_t rv;
  struct arp_entry *entry;
  struct arp_entry *dentry;
  char gw[BUFSIZ];

  rv = arp_delete_entry(ifindex, dst_addr, ll_addr);
  if (rv != LAGOPUS_RESULT_OK && rv != LAGOPUS_RESULT_NOT_FOUND) {
    return rv;
  }

  entry = calloc(1, sizeof(struct arp_entry));
  if (entry == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  entry->ifindex = ifindex;
  memcpy(entry->mac_addr, ll_addr, ETHER_ADDR_LEN);

  dentry = entry;
  rv = lagopus_hashmap_add(&arp_entry_hashmap, (void *)dst_addr->s_addr,
                            (void **)&dentry, true);
  return rv;
}

static lagopus_result_t
arp_find_entry(struct in_addr *addr, char *mac, int *ifindex) {
  lagopus_result_t rv;
  struct arp_entry *arp;
  char gw[BUFSIZ] = {0};

  lagopus_rwlock_reader_enter_critical(&arp_lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&arp_entry_hashmap,
                            (void *)(addr->s_addr), (void **)&arp);
  if (arp != NULL && rv == LAGOPUS_RESULT_OK) {
    mac[0] = arp->mac_addr[0];
    mac[1] = arp->mac_addr[1];
    mac[2] = arp->mac_addr[2];
    mac[3] = arp->mac_addr[3];
    mac[4] = arp->mac_addr[4];
    mac[5] = arp->mac_addr[5];
    *ifindex = arp->ifindex;
  } else {
    PRINTF("arp no entry.\n");
    *ifindex = -1;
  }
  (void)lagopus_rwlock_leave_critical(&arp_lock, cstate);
}

/* public apis */
void
arp_init(void) {
  lagopus_rwlock_create(&arp_lock);
  lagopus_hashmap_create(&arp_entry_hashmap,
                         LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                         arp_entry_free);
}

void
arp_fini(void) {
  if (&arp_entry_hashmap != NULL) {
    lagopus_hashmap_destroy(&arp_entry_hashmap, true);
  }
  lagopus_rwlock_destroy(&arp_lock);
}

void
arp_get(struct in_addr *addr, char *mac, int *ifindex) {
  arp_find_entry(addr, mac, ifindex);
}

void
arp_add(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  arp_update_entry(ifindex, dst_addr, ll_addr);
  arp_log("add", ifindex, dst_addr, ll_addr);
}

void
arp_delete(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  arp_delete_entry(ifindex, dst_addr, ll_addr);
  arp_log("del", ifindex, dst_addr, ll_addr);
}

