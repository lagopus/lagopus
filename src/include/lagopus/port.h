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
 *      @file   port.h
 */

#ifndef SRC_INCLUDE_LAGOPUS_PORT_H_
#define SRC_INCLUDE_LAGOPUS_PORT_H_

struct port_stats_list;
struct port_desc_list;
struct port_stats;

struct bridge;
struct interface;
struct lagopus_packet;

/**
 * @brief Port structure.
 */
struct port {
  int type;                             /** Port type. */
  struct ofp_port ofp_port;             /** OpenFlow port. */
  struct ofp_port_stats ofp_port_stats; /** OpenFlow port statistics. */
  uint32_t ifindex;                     /** Interface index for physical port.
                                         *  Not same as ofp_port.port_no. */
  struct bridge *bridge;                /** Parent bridge. */
  struct interface *interface;          /** Related interface. */
  lagopus_hashmap_t queueinfo_hashmap;  /** Related active queue hashtable. */
  lagopus_bbq_t pcap_queue;             /** Packet queue for capture */
  struct timespec create_time;          /** Creation time. */

  /* Type specific member. */
  union {
    struct {
      struct {
        uint16_t domain;
        uint8_t bus;
        uint8_t devid;
        uint8_t function;
      } pci_addr;
      struct {
        uint16_t vendor_id;
        uint16_t device_id;
      } pci_id;
    } eth;
    struct {
      in_addr_t tunnel_local_ip;
      in_addr_t tunnel_remote_ip;
    } gre;
  };
};

lagopus_result_t (*lagopus_register_port_hook)(struct port *);
lagopus_result_t (*lagopus_change_port_hook)(struct port *);
lagopus_result_t (*lagopus_unregister_port_hook)(struct port *);

/**
 * Alloc internal port structure.
 *
 * @retval      !=NULL  Port structure.
 * @retval      ==NULL  Memory exhausted.
 */
struct port *
port_alloc(void);

/**
 * Free internal port structure.
 *
 * @param[in]   port    Port structure.
 */
void
port_free(struct port *);

/**
 * Lookup port by number.
 *
 * @param[in]   v       Port database.
 * @param[in]   port_no Port number depend on database.
 *
 * @retval      !=NULL  Port object.
 * @retval      ==NULL  Port is not exist.
 *
 * port_no is depend on database type of v.
 * - If type is OpenFlow, port_no allows OpenFlow port number.
 * - If type is Physical port, port_no allows port id e.g. ifindex.
 */
struct port *
port_lookup(lagopus_hashmap_t *hm, uint32_t port_no);

/**
 * Get port statistics.
 *
 * @param[in]   ports   Port database.
 * @param[in]   request Request data includes port number.
 * @param[out]  list    List of port statistics.
 * @param[out]  error   Error type and code.
 *
 * @retval      LAGOPUS_RESULT_OK       Succeeded.
 * @retval      !=LAGOPUS_RESULT_OK     Failed.
 */
lagopus_result_t
lagopus_get_port_statistics(lagopus_hashmap_t *hm,
                            struct ofp_port_stats_request *request,
                            struct port_stats_list *list,
                            struct ofp_error *error);

/**
 * Process port_mod request.
 *
 * @param[in]   bridge          Bridge.
 * @param[in]   port_mod        OpenFlow port_mod request.
 * @param[out]  error           Error indicated if error in port_mod.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
port_config(struct bridge *bridge,
            struct ofp_port_mod *port_mod,
            struct ofp_error *error);

/**
 * Get port description.
 *
 * @param[in]   ports   Port database.
 * @param[out]  list    List of port description.
 * @param[out]  error   Error indicated if error detected.
 */
lagopus_result_t
get_port_desc(lagopus_hashmap_t *hm,
              struct port_desc_list *list,
              struct ofp_error *error);

/**
 * Get port liveness.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   port_no OpenFlow port number.
 *
 * @retval      true    Live.
 * @retval      false   Dead.
 *
 * So far, live means interface up.  dead means down.
 * In future, get interface state if STP or other protocols are implemented.
 */
bool
port_liveness(struct bridge *bridge, uint32_t port_no);

enum {
  /* null ports for test */
  LAGOPUS_PORT_TYPE_NULL = 0,

  /* physical ports */
  LAGOPUS_PORT_TYPE_PHYSICAL,   /* eth* */
  /* logical ports */
  LAGOPUS_PORT_TYPE_VLAN,
  LAGOPUS_PORT_TYPE_GRE,
  LAGOPUS_PORT_TYPE_VXLAN,
  LAGOPUS_PORT_TYPE_NVGRE,
  /* reserved ports */
  LAGOPUS_PORT_TYPE_RESERVED,

  LAGOPUS_PORT_TYPE_MAX
};

/* Prototypes. */
void lagopus_send_packet(uint32_t, struct lagopus_packet *);
void dp_port_update_link_status(struct port *port);
lagopus_result_t dpdk_update_port_link_status(struct port *port);

#endif /* SRC_INCLUDE_LAGOPUS_PORT_H_ */
