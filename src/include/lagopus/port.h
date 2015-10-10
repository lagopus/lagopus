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
 *      @file   port.h
 */

#ifndef SRC_INCLUDE_LAGOPUS_PORT_H_
#define SRC_INCLUDE_LAGOPUS_PORT_H_

struct port_stats_list;
struct port_desc_list;
struct port_stats;

struct bridge;
struct interface;

/**
 * Port structure.
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
  struct port_stats *(*stats)(struct port *);  /** Statistics function. */

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
 * Create port database.
 *
 * @retval      !=NULL  Port database.
 * @retval      ==NULL  Memory exhausted.
 */
struct vector *
ports_alloc(void);

/**
 * Destroy port database.
 *
 * @param[in]   v       Port database.
 */
void
ports_free(struct vector *v);

struct port *
port_alloc(void);

void
port_free(struct port *);

/**
 * Lookup port.
 *
 * @param[in]   v       Port database.
 * @param[in]   port_no OpenFlow port number.
 *
 * @retval      !=NULL  Port object.
 * @retval      ==NULL  Port is not exist.
 */
struct port *
port_lookup(struct vector *v, uint32_t port_no);

/**
 * get port structure by OpenFlow flow number.
 *
 * @param[in]   vector owned by dpmgr
 * @param[in]   port_no OpenFlow Port number.
 * @retval      !=NULL  lagopus port object
 *              ==NULL  lagopus port is not configured
 */
struct port *
port_lookup_number(struct vector *v, uint32_t port_no);

/**
 * Count number of ports.
 *
 * @param[in]   v               Port database.
 *
 * @retval      Number of ports.
 */
unsigned int
num_ports(struct vector *v) __attribute__ ((deprecated));

/**
 * get port statistics.
 *
 * @param[in]   ports   Port database.
 * @param[in]   request request data includes port number.
 * @param[out]  list    list of port statistics.
 * @param[out]  error   error type and code.
 *
 * @retval      LAGOPUS_RESULT_OK       Succeeded.
 * @retval      !=LAGOPUS_RESULT_OK     Failed
 */
lagopus_result_t
lagopus_get_port_statistics(struct vector *ports,
                            struct ofp_port_stats_request *request,
                            struct port_stats_list *list,
                            struct ofp_error *error);

/**
 */
lagopus_result_t
port_config(struct bridge *bridge,
            struct ofp_port_mod *port_mod,
            struct ofp_error *error);

/**
 */
lagopus_result_t
get_port_desc(struct vector *ports,
              struct port_desc_list *list,
              struct ofp_error *error);

/**
 */
bool
port_liveness(struct bridge *bridge, uint32_t port_no);

/**
 * outout queue per port
 * see 7.2.2 Queue Structures
 */
struct lagopus_queue {
  TAILQ_ENTRY(lagopus_queue) next;
  uint32_t queue_id;
  uint16_t min_rate;
  uint16_t max_rate;
  /* struct lagopus_packet_list tx_queue; */
};

TAILQ_HEAD(lagopus_queue_list, lagopus_queue);

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

struct bridge;

struct lagopus_packet;

/* Prototypes. */
struct port *ifindex2port(struct vector *, uint32_t);
void lagopus_send_packet(uint32_t, struct lagopus_packet *);

#endif /* SRC_INCLUDE_LAGOPUS_PORT_H_ */
