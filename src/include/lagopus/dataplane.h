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
 *      @file   dataplane.h
 *      @brief  Dataplane APIs
 */

#ifndef SRC_INCLUDE_LAGOPUS_DATAPLANE_H_
#define SRC_INCLUDE_LAGOPUS_DATAPLANE_H_

#ifdef HAVE_DPDK
#include <rte_ether.h>
#else
#include <net/ethernet.h>
#endif /* HAVE_DPDK */
#include <netinet/in.h>

#include "lagopus_thread.h"
#include "lagopus_gstate.h"
#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/flowinfo.h"
#include "lagopus/port.h"
#include "lagopus/interface.h"

#define LAGOPUS_DATAPLANE_VERSION "0.9"

/*
 * OpenFlow support level configuration.
 */
#define HAVE_IPV6
#undef HAVE_OF_QUEUE
#define PBB_IS_VLAN
#undef PACKET_CAPTURE

#define __UNUSED __attribute__((unused))

#undef DP_DEBUG
#ifdef DP_DEBUG
#define DP_PRINT(...) printf(__VA_ARGS__)
#define DP_PRINT_HEXDUMP(ptr, len)                \
  do {                                          \
    uint8_t *p_ = (uint8_t *)(ptr);             \
    int i_;                                     \
    for (i_ = 0; i_ < (len); i_++) {            \
      printf(" %02x", p_[i_]);                  \
    }                                           \
  } while (0)
#define DPRINT_FLOW(flow) flow_dump(flow, stdout)
#else
#define DP_PRINT(...)
#define DP_PRINT_HEXDUMP(ptr, len)
#define DPRINT_FLOW(flow)
#endif

struct flow;
struct table;
struct lagopus_packet;
struct meter;
struct port;
struct bucket_list;
struct bucket;

struct dataplane_arg {
  lagopus_thread_t *threadptr;
  lagopus_mutex_t *lock;
  int argc;
  char **argv;
  bool *running;
};

int
dpdk_send_packet_physical(struct lagopus_packet *pkt, struct interface *);
int
rawsock_send_packet_physical(struct lagopus_packet *pkt, uint32_t portid);


/* Inline functions. */
/**
 * calc packet header size.
 * it like packet classification.
 */
static inline size_t
calc_packet_header_size(const char *pkt_data, size_t len) {
  const uint8_t *s, *p;
  uint16_t ether_type;
  uint8_t proto;

  s = p = (uint8_t *)pkt_data;
  p += ETHER_ADDR_LEN * 2;
  while (p - s < (int)len) {
    ether_type = (uint16_t)(*p << 8 | *(p +1));
    p += ETHER_TYPE_LEN;
  skip:
    switch (ether_type) {
      case ETHERTYPE_VLAN:
      case 0x88a8:
        p += 2;
        break;
      case ETHERTYPE_ARP:
        p += 8 + 6 + 4 + 6 + 4;
        goto done;
      case ETHERTYPE_IP:
        proto = *(p + 9);
        if ((*p & 0x0f) == 0) {
          goto done;
        }
        p += (*p & 0x0f) << 2;
      ipproto:
        switch (proto) {
          case IPPROTO_ICMP:
          case IPPROTO_ICMPV6:
            /* OpenFlow look up type and code. */
            p += 2;
            goto done;
          case IPPROTO_TCP:
            /* OpenFlow look up src and dst port. */
            p += 4;
            goto done;
          case IPPROTO_UDP:
            /* OpenFlow look up src and dst port. */
#if 0
            if (port == vxlan) {
              p += sizeof udphdr;
              p += vxlanhdr;
              break;
            }
#endif
            p += 4;
            goto done;
          case IPPROTO_SCTP:
            /* OpenFlow look up src and dst port. */
            p += 4;
            goto done;
#if 0
          case IPPROTO_GRE:
            p += 4; /* XXX depend on flags */
            ether_type = ETHERTYPE_IP;
            goto skip;
#endif
          case IPPROTO_HOPOPTS:
          case IPPROTO_ROUTING:
          case IPPROTO_ESP:
          case IPPROTO_DSTOPTS:
            proto = *p;
            if (p[1] == 0) {
              goto done;
            }
            p += p[1] << 3;
            goto ipproto;
          case IPPROTO_FRAGMENT:
            proto = *p;
            p += 8;
            goto ipproto;
          case IPPROTO_AH:
            proto = *p;
            if (p[1] == 0) {
              goto done;
            }
            p += p[1] << 2;
            goto ipproto;
          case IPPROTO_NONE:
            proto = *p;
            p += 2;
            goto ipproto;
          default:
            goto done;
        }
        break;
      case ETHERTYPE_IPV6:
        proto = p[6];
        p += 8 + 16 + 16;
        goto ipproto;
      case ETHERTYPE_MPLS:
      case ETHERTYPE_MPLS_MCAST:
        if ((p[2] & 1) == 0) {
          /* not bos */
          ether_type = ETHERTYPE_MPLS;
        } else {
          /* inner ether type is not exist in packet.  guess it. */
          if (p[4] >= 0x45 && p[4] <= 0x4f) {
            ether_type = ETHERTYPE_IP;
          } else if (p[4] >= 0x60 && p[4] <= 0x6f) {
            ether_type = ETHERTYPE_IPV6;
          } else if (p[4] == 0x00 && p[5] == 0x01 &&
                     p[6] == 0x00 && p[7] == 0x80) {
            ether_type = ETHERTYPE_ARP;
          } else if ((p[6] == 0x08 && p[7] == 0x00 &&
                      p[8] >= 0x45 && p[8] <= 0x4f) ||
                     (p[6] == 0x86 && p[7] == 0xdd &&
                      p[8] >= 0x45 && p[8] <= 0x4f) ||
                     (p[6] == 0x08 && p[7] == 0x06 &&
                      p[8] == 0x00 && p[9] == 0x80) ||
                     (p[6] == 0x81 && p[7] == 0x00) ||
                     (p[6] == 0x88 && p[7] == 0xa8)) {
            /* L3 over VLAN */
            ether_type = (uint16_t)((p[6] << 8) | p[7]);
            p += 2;
          } else {
            /* guess other protocols? */
            p += 4;
            goto done;
          }
        }
        p += 4;
        goto skip;
      case ETHERTYPE_PBB:
        p += 16;
        break;
      default:
        goto done;
    }
  }
done:
  return (size_t)(p - s);
}

lagopus_result_t
execute_instruction_meter(struct lagopus_packet *pkt,
                          const struct instruction *instruction);
lagopus_result_t
execute_instruction_apply_actions(struct lagopus_packet *pkt,
                                  const struct instruction *instruction);
lagopus_result_t
execute_instruction_clear_actions(struct lagopus_packet *pkt,
                                  const struct instruction *insn);
lagopus_result_t
execute_instruction_write_actions(struct lagopus_packet *pkt,
                                  const struct instruction *instruction);
lagopus_result_t
execute_instruction_write_metadata(struct lagopus_packet *pkt,
                                   const struct instruction *instruction);
lagopus_result_t
execute_instruction_goto_table(struct lagopus_packet *pkt,
                               const struct instruction *instruction);
lagopus_result_t
execute_instruction_experimenter(struct lagopus_packet *pkt,
                                 const struct instruction *instruction);
lagopus_result_t
execute_instruction_none(struct lagopus_packet *pkt,
                         const struct instruction *instruction);

/**
 * Execute OpenFlow Instruction.
 *
 * @param[in]   pkt             packet.
 * @param[in]   instructions    instructions.
 *
 * @retval      0       executed instruction successfully.
 */
static inline lagopus_result_t
execute_instruction(struct lagopus_packet *pkt,
                    const struct instruction **instructions) {
  const struct instruction *instruction;
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;
  instruction = *instructions++;
  if (instruction != NULL) {
    DP_PRINT_HEXDUMP(&instruction->ofpit + 1, instruction->ofpit.len - 4);
    DP_PRINT("\n");
    rv = execute_instruction_meter(pkt, instruction);
    if (rv != LAGOPUS_RESULT_OK) {
      DP_PRINT("stop execution.\n");
      goto out;
    }
  }
  instruction = *instructions++;
  if (instruction != NULL) {
    DP_PRINT_HEXDUMP(&instruction->ofpit + 1, instruction->ofpit.len - 4);
    DP_PRINT("\n");
    rv = execute_instruction_apply_actions(pkt, instruction);
    if (rv != LAGOPUS_RESULT_OK) {
      DP_PRINT("stop execution.\n");
      goto out;
    }
  }
  instruction = *instructions++;
  if (instruction != NULL) {
    DP_PRINT_HEXDUMP(&instruction->ofpit + 1, instruction->ofpit.len - 4);
    DP_PRINT("\n");
    rv = execute_instruction_clear_actions(pkt, instruction);
    if (rv != LAGOPUS_RESULT_OK) {
      DP_PRINT("stop execution.\n");
      goto out;
    }
  }
  instruction = *instructions++;
  if (instruction != NULL) {
    DP_PRINT_HEXDUMP(&instruction->ofpit + 1, instruction->ofpit.len - 4);
    DP_PRINT("\n");
    rv = execute_instruction_write_actions(pkt, instruction);
    if (rv != LAGOPUS_RESULT_OK) {
      DP_PRINT("stop execution.\n");
      goto out;
    }
  }
  instruction = *instructions++;
  if (instruction != NULL) {
    DP_PRINT_HEXDUMP(&instruction->ofpit + 1, instruction->ofpit.len - 4);
    DP_PRINT("\n");
    rv = execute_instruction_write_metadata(pkt, instruction);
    if (rv != LAGOPUS_RESULT_OK) {
      DP_PRINT("stop execution.\n");
      goto out;
    }
  }
  instruction = *instructions++;
  if (instruction != NULL) {
    DP_PRINT_HEXDUMP(&instruction->ofpit + 1, instruction->ofpit.len - 4);
    DP_PRINT("\n");
    rv = execute_instruction_goto_table(pkt, instruction);
    if (rv != LAGOPUS_RESULT_OK) {
      DP_PRINT("stop execution.\n");
      goto out;
    }
  }
out:
  return rv;
}

/* Prototypes. */

/**
 * Allocate lagopus packet structure with packet data buffer.
 *
 * @retval      !=NULL  pointer of allocated packet structure.
 *              ==NULL  failed to allocate.
 */
struct lagopus_packet *alloc_lagopus_packet(void);

/**
 * Free data structure associated with the packet.
 *
 * @param[in]   pkt     Packet.
 *
 * decrement refcnt and free pkt->mbuf if refcnt is zero.
 */
void lagopus_packet_free(struct lagopus_packet *);

/**
 * Prepare received packet information.
 *
 * @param[in]   pkt     packet structure.
 * @param[in]   m       pointer to data of the packet.
 *                      such as mbuf, skbuff, etc.
 * @param[in]   port    internal port structure.
 */
void lagopus_packet_init(struct lagopus_packet *, void *, struct port *);

/**
 * Send packet to specified OpenFlow port.
 *
 * @param[in]   pkt     packet.
 * @param[in]   port    OpenFlow port number.
 */
void lagopus_forward_packet_to_port(struct lagopus_packet *, uint32_t);

/**
 * Process packet by OpenFlow rule.
 *
 * @param[in]   pkt     packet.
 *
 * @retval      LAGOPUS_RESULT_OK              Should be freed packet buffer.
 * @retval      LAGOPUS_RESULT_NO_MORE_ACTION  Don't free packet buffer.
 * @retval      LAGOPUS_RESULT_STOP            Allow stop pipeline.
 *
 * lookup flow table from bridge related with ingress port of packet.
 * flow table id is in the packet.
 */
lagopus_result_t lagopus_match_and_action(struct lagopus_packet *);

/**
 * Execute experimenter instruction.
 *
 * @param[in]   pkt     packet.
 * @param[in]   exp_id  experimenter id.
 *
 * so far, empty function definition.
 */
void lagopus_instruction_experimenter(struct lagopus_packet *, uint32_t);

/**
 * Create/Merge action-set.  Do write actions.
 *
 * @param[out]  actions         Action set.
 * @param[in]   action_list     action list.
 *
 * @retval      LAGOPUS_RESULT_OK       success.
 *
 * actlin_list is appended to internal action list in the packet.
 */
lagopus_result_t
merge_action_set(struct action_list *actions,
                 const struct action_list *action_list);

/**
 * Execute action.
 *
 * @param[in]   pkt             packet.
 * @param[in]   action_list     action list.
 *
 * @retval      LAGOPUS_RESULT_OK       success.
 * @retval      LAGOPUS_RESULT_STOP     stop execution.
 *
 * action->flags are hint of omit copying part of header.
 *      SET_FIELD_ETH_SRC       set-field eth_src after push action.
 *                              do not need copy eth_src by push action.
 *      SET_FIELD_ETH_DST       set-field eth_dst after push action.
 *                              do not need copy eth_dst by push action.
 */
static inline lagopus_result_t
execute_action(struct lagopus_packet *pkt,
               const struct action_list *action_list) {
  struct action *action;
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;
  TAILQ_FOREACH(action, action_list, entry) {
    DP_PRINT("action type:%d len:%d", action->ofpat.type, action->ofpat.len);
    DP_PRINT_HEXDUMP(action->ofpat.pad, action->ofpat.len - 4);
    DP_PRINT("\n");

    if ((rv = action->exec(pkt, action)) != LAGOPUS_RESULT_OK) {
      break;
    }
  }
  return rv;
}

/**
 * Send packet to specified physical port.
 *
 * @param[in]   pkt     packet.
 * @param[in]   port    physical port number (ifindex).
 *
 */
static inline int
lagopus_send_packet_physical(struct lagopus_packet *pkt,
                             struct interface *ifp) {
  if (ifp == NULL) {
    return LAGOPUS_RESULT_OK;
  }
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_send_packet_physical(pkt, ifp);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_send_packet_physical(pkt,
                                          ifp->info.eth_rawsock.port_number);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
      /* TODO */
      lagopus_packet_free(pkt);
      return LAGOPUS_RESULT_OK;
    default:
      break;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Send packet to kernel normal path related with physical port.
 *
 * @param[in]   pkt     packet.
 * @param[in]   port    physical port number (ifindex).
 *
 */
int lagopus_send_packet_normal(struct lagopus_packet *, uint32_t);

/**
 * Return if port is using by lagopus vswitch.
 *
 * @param[in]   port    port.
 *
 * @retval      true    using by lagopus vswitch.
 * @retval      false   don't used by lagopus vswitch.
 */
bool lagopus_is_port_enabled(const struct port *);

/**
 * Return if portid is using by lagopus vswitch.
 *
 * @param[in]   portid  physical port number.
 *
 * @retval      true    using by lagopus vswitch.
 * @retval      false   don't used by lagopus vswitch.
 */
bool lagopus_is_portid_enabled(int);


/**
 * Select bucket of packet.
 *
 * @param[in]   pkt             Packet.
 * @param[in]   list            Bucket list.
 *
 * @retval      NULL            bucket list is empty.
 * @retval      !=NULL          selected bucket.
 */
struct bucket *
group_select_bucket(struct lagopus_packet *pkt, struct bucket_list *list);

/**
 * initialize meter support in lower driver.
 */
void lagopus_meter_init(void);

/**
 * metering packet.
 *
 * @param[in]   pkt             packet.
 * @param[in]   meter           meter structure.
 * @param[out]  prec_level      new DSCP level.
 *
 * @retval      0                       normal output.
 * @retval      OFPMBT_DROP             should be dropped packet.
 * @retval      OFPMBT_DSCP_REMARK      should be remark DSCP.
 * @retval      OFPMBT_EXPERIMENTER     experimenter.
 */
int lagopus_meter_packet(struct lagopus_packet *, struct meter *, uint8_t *);

/**
 * Configure physical port.
 *
 * @param[in]   port    physical port.
 *
 * @retval      LAGOPUS_RESULT_OK       success.
 */
lagopus_result_t
lagopus_configure_physical_port(struct port *);

/**
 * Unconfigure physical port.
 *
 * @param[in]   port    physical port.
 *
 * @retval      LAGOPUS_RESULT_OK       success.
 */
lagopus_result_t
lagopus_unconfigure_physical_port(struct port *);

/**
 * Set switch configuration.
 */
lagopus_result_t
lagopus_set_switch_config(struct bridge *bridge,
                          struct ofp_switch_config *config,
                          struct ofp_error *error);

/**
 * Get switch configuration.
 */
lagopus_result_t
lagopus_get_switch_config(struct bridge *bridge,
                          struct ofp_switch_config *config,
                          struct ofp_error *error);

/**
 * initialize dataplane.  e.g. setup Intel DPDK.
 *
 * @param[in]   argc                    argc from command line
 * @param[in]   argv                    argv from commend line
 * @retval      LAGOPUS_RESULT_OK       initialized successfully.
 * @retval      !=LAGOPUS_RESULT_OK     failed to initialize.
 *
 * Intel DPDK version note:
 * This function is to be executed on the MASTER lcore only,
 * as soon as possible in the application's main() function.
 */
lagopus_result_t
lagopus_dataplane_init(int argc, const char * const argv[]);

/**
 */
lagopus_result_t
rawsock_dataplane_init(int argc, const char *const argv[]);

/**
 */
lagopus_result_t
dataplane_initialize(int argc, const char *const argv[],
                     void *extarg, lagopus_thread_t **thdptr);

/**
 */
void dataplane_finalize(void);

/**
 * loop function wait for finish all DPDK thread.
 */
lagopus_result_t
dpdk_thread_loop(const lagopus_thread_t *, void *);

/**
 * dataplane start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 *
 * Intel DPDK version note:
 * This function is to be executed on the MASTER lcore only.
 */
lagopus_result_t dataplane_start(void);

/**
 * dataplane shutdown function.
 */
lagopus_result_t dataplane_shutdown(shutdown_grace_level_t);

/**
 * dataplane stop function.
 */
lagopus_result_t dataplane_stop(void);

/**
 * Dataplane communicator thread
 */
lagopus_result_t
dpcomm_initialize(int argc,
                  const char *const argv[],
                  void *extarg,
                  lagopus_thread_t **thdptr);

/**
 * Dataplane communicator thread
 */
lagopus_result_t
dpcomm_start(void);

/**
 * Dataplane communicator thread
 */
void
dpcomm_finalize(void);

/**
 * Dataplane communicator thread
 */
lagopus_result_t
dpcomm_shutdown(shutdown_grace_level_t level);

/**
 * Dataplane communicator thread
 */
lagopus_result_t
dpcomm_stop(void);

/**
 * dataplane communicator loop function.
 */
lagopus_result_t
comm_thread_loop(const lagopus_thread_t *, void *);

/**
 */
void lagopus_set_action_function(struct action *);

/**
 */
void lagopus_set_instruction_function(struct instruction *);

/**
 */
lagopus_result_t send_port_status(struct port *, uint8_t);

/**
 */
lagopus_result_t update_port_link_status(struct port *);

/**
 * Copy packet.
 *
 * @param[in]   pkt     Packet.
 *
 * @retval      Copied packet.
 */
struct lagopus_packet *copy_packet(struct lagopus_packet *);

/**
 */
void copy_dataplane_info(char *buf, int len);

/**
 * Raw socket I/O process function.
 *
 * @param[in]   t       Thread object pointer.
 * @param[in]   arg     Do not used argument.
 */
lagopus_result_t rawsock_thread_loop(const lagopus_thread_t *t, void *arg);

/**
 * Print usage.
 *
 * @param[in]   fp      Output file pointer.
 */
void dataplane_usage(FILE *fp);

bool rawsocket_only_mode;

#endif /* SRC_INCLUDE_LAGOPUS_DATAPLANE_H_ */
