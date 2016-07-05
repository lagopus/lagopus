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
 *      @file   dataplane.h
 *      @brief  Dataplane APIs
 */

#ifndef SRC_INCLUDE_LAGOPUS_DATAPLANE_H_
#define SRC_INCLUDE_LAGOPUS_DATAPLANE_H_

#include "lagopus_thread.h"
#include "lagopus_gstate.h"
#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/flowinfo.h"
#include "lagopus/port.h"

struct interface;

#define LAGOPUS_DATAPLANE_VERSION "0.9"

#undef PBB_UCA_SUPPORT
#define GENERAL_TUNNEL_SUPPORT

#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif /* IPPROTO_SCTP */

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

int
dpdk_send_packet_physical(struct lagopus_packet *pkt, struct interface *);
int
rawsock_send_packet_physical(struct lagopus_packet *pkt, uint32_t portid);

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

#ifdef HYBRID
void
lagopus_forward_packet_to_port_hybrid(struct lagopus_packet *);
#endif /* HYBRID */

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
int
lagopus_send_packet_physical(struct lagopus_packet *pkt,
                             struct interface *ifp);

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
 * initialize Intel DPDK related APIs.
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
dpdk_dataplane_init(int argc, const char * const argv[]);

/**
 */
lagopus_result_t
rawsock_dataplane_init(int argc, const char *const argv[]);

/**
 * Dataplane rawsocket thread initialization.
 */
lagopus_result_t
dp_rawsock_thread_init(int argc, const char *const argv[],
                       void *extarg, lagopus_thread_t **thdptr);

/**
 * Dataplane rawsock thread start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 */
lagopus_result_t dp_rawsock_thread_start(void);

/**
 * Dataplane rawsock thread stop function.
 */
lagopus_result_t dp_rawsock_thread_stop(void);

/**
 * Dataplane rawsock thread shutdown function.
 */
lagopus_result_t dp_rawsock_thread_shutdown(shutdown_grace_level_t);

/**
 * Dataplane rawsock thread finalize function.
 */
void dp_rawsock_thread_fini(void);

/**
 * Dataplane dpdk thread initialization.
 */
lagopus_result_t
dp_dpdk_thread_init(int argc, const char *const argv[],
                       void *extarg, lagopus_thread_t **thdptr);

/**
 * Dataplane dpdk thread start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 */
lagopus_result_t dp_dpdk_thread_start(void);

/**
 * Dataplane dpdk thread stop function.
 */
lagopus_result_t dp_dpdk_thread_stop(void);

/**
 * Dataplane dpdk thread shutdown function.
 */
lagopus_result_t dp_dpdk_thread_shutdown(shutdown_grace_level_t);

/**
 * Dataplane dpdk thread finalize function.
 */
void dp_dpdk_thread_fini(void);

/**
 * Dataplane dpdk thread usage function.
 */
void dp_dpdk_thread_usage(FILE *fp);

/**
 * Dataplane comm thread initialization.
 */
lagopus_result_t
dp_comm_thread_init(int argc, const char *const argv[],
                       void *extarg, lagopus_thread_t **thdptr);

/**
 * Dataplane comm thread start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 */
lagopus_result_t dp_comm_thread_start(void);

/**
 * Dataplane comm thread stop function.
 */
lagopus_result_t dp_comm_thread_stop(void);

/**
 * Dataplane comm thread shutdown function.
 */
lagopus_result_t dp_comm_thread_shutdown(shutdown_grace_level_t);

/**
 * Dataplane comm thread finalize function.
 */
void dp_comm_thread_fini(void);

/**
 * Dataplane timer thread initialization.
 */
lagopus_result_t
dp_timer_thread_init(int argc, const char *const argv[],
                       void *extarg, lagopus_thread_t **thdptr);

/**
 * Dataplane timer thread start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 */
lagopus_result_t dp_timer_thread_start(void);

/**
 * Dataplane timer thread stop function.
 */
lagopus_result_t dp_timer_thread_stop(void);

/**
 * Dataplane timer thread shutdown function.
 */
lagopus_result_t dp_timer_thread_shutdown(shutdown_grace_level_t);

/**
 * Dataplane timer thread finalize function.
 */
void dp_timer_thread_fini(void);

#ifdef HYBRID
/**
 * Dataplane tapio thread initialization.
 */
lagopus_result_t
dp_tapio_thread_init(int argc, const char *const argv[],
                       void *extarg, lagopus_thread_t **thdptr);

/**
 * Dataplane tapio thread start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 */
lagopus_result_t dp_tapio_thread_start(void);

/**
 * Dataplane tapio thread stop function.
 */
lagopus_result_t dp_tapio_thread_stop(void);

/**
 * Dataplane tapio thread shutdown function.
 */
lagopus_result_t dp_tapio_thread_shutdown(shutdown_grace_level_t);

/**
 * Dataplane tapio thread finalize function.
 */
void dp_tapio_thread_fini(void);

/**
 * Dataplane netlink thread initialization.
 */
lagopus_result_t
dp_netlink_thread_init(int argc, const char *const argv[],
                       void *extarg, lagopus_thread_t **thdptr);

/**
 * Dataplane netlink thread start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 */
lagopus_result_t dp_netlink_thread_start(void);

/**
 * Dataplane netlink thread stop function.
 */
lagopus_result_t dp_netlink_thread_stop(void);

/**
 * Dataplane netlink thread shutdown function.
 */
lagopus_result_t dp_netlink_thread_shutdown(shutdown_grace_level_t);

/**
 * Dataplane netlink thread finalize function.
 */
void dp_netlink_thread_fini(void);

#ifdef PIPELINER
/**
 * Legacy L2/L3 switch thread initialization.
 */
lagopus_result_t
dp_legacy_sw_thread_init(int argc, const char *const argv[],
                         void *extarg, lagopus_thread_t **thdptr);

/**
 * Dataplane netlink thread start function.
 *
 * @retval      LAGOPUS_RESULT_OK       dataplane main thread is created.
 * @retval      !=LAGOPUS_RESULT_OK     dataplane main thread is not created.
 */
lagopus_result_t dp_legacy_sw_thread_start(void);

/**
 * Dataplane netlink thread stop function.
 */
lagopus_result_t dp_legacy_sw_thread_stop(void);

/**
 * Dataplane netlink thread shutdown function.
 */
lagopus_result_t dp_legacy_sw_thread_shutdown(shutdown_grace_level_t);

/**
 * Dataplane netlink thread finalize function.
 */
void dp_legacy_sw_thread_fini(void);
#endif /* PIPELINER */
#endif /* HYBRID */

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
 * Print usage.
 *
 * @param[in]   fp      Output file pointer.
 */
void dataplane_usage(FILE *fp);

#ifdef HYBRID
lagopus_result_t
lagopus_rewrite_pkt_header(struct lagopus_packet *pkt, uint8_t *src, uint8_t *dst);

uint32_t
lagopus_get_ethertype(struct lagopus_packet *pkt);

/**
 * Get IPv4 address in packet.
 *
 * @param[in]  pkt  Packet.
 * @param[out] dst  IPv4 address.
 */
lagopus_result_t
lagopus_get_ip(struct lagopus_packet *pkt, void *dst, const int family);

uint32_t
dpdk_get_worker_id(void);
#endif /* HYBRID */
#endif /* SRC_INCLUDE_LAGOPUS_DATAPLANE_H_ */
