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
 *      @file   flowinfo.h
 *      @brief  Flow information for fast lookup.
 */

#ifndef SRC_INCLUDE_LAGOPUS_FLOWINFO_H_
#define SRC_INCLUDE_LAGOPUS_FLOWINFO_H_

/**
 * @brief Structured flow table.
 */
struct flowinfo {
  int nflow;                    /** number of entries. */
  unsigned int nnext;           /** number of child flowinfo. */
  union {                       /** entries includes type specific match. */
    lagopus_hashmap_t hashmap;  /** hashmap entries. */
    struct flow **flows;        /** simple array entries. */
    struct flowinfo **next;     /** child flowinfo array. */
    /* add more types if needed. */
  };
  struct flowinfo *misc;        /** flowinfo includes no specific match. */
  uint64_t userdata;            /** type specific data placeholder. */

  lagopus_result_t (*add_func)(struct flowinfo *, struct flow *);
  struct flow *(*match_func)(struct flowinfo *, struct lagopus_packet *,
                             int32_t *);
  lagopus_result_t (*del_func)(struct flowinfo *, struct flow *);
  struct flow *(*find_func)(struct flowinfo *, struct flow *);
  void (*destroy_func)(struct flowinfo *);
};

/**
 * Allocate and initialize flowinfo for sequencial search.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_basic(void);

/**
 * Allocate and initialize flowinfo for ingress port.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_in_port(void);

/**
 * Allocate and initialize flowinfo for metadata.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_metadata(void);

/**
 * Allocate and initialize flowinfo for VLAN VID.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_vlan_vid(void);

/**
 * Allocate and initialize flowinfo for ethernet type.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_eth_type(void);

/**
 * Allocate and initialize flowinfo for IPv4 destination address with mask.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_ipv4_dst_mask(void);

/**
 * Allocate and initialize flowinfo for IPv4 destination address.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_ipv4_dst(void);

/**
 * Allocate and initialize flowinfo for IPv4 source address with mask.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_ipv4_src_mask(void);

/**
 * Allocate and initialize flowinfo for IPv4 source address.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_ipv4_src(void);

/**
 * Allocate and initialize flowinfo for IPv4 packets.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_ipv4(void);

/**
 * Allocate and initialize flowinfo for IPv6 packets.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_ipv6(void);

/**
 * Allocate and initialize flowinfo for MPLS packets.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_mpls(void);

/**
 * Allocate and initialize flowinfo for metadata.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_metadata(void);

/**
 * Allocate and initialize flowinfo for metadata with mask.
 *
 * @retval      !=NULL  Created flowinfo.
 *              ==NULL  failed to create flowinfo.
 */
struct flowinfo *new_flowinfo_metadata_mask(void);

/**
 * Initialize flowinfo module.
 */
void flowinfo_init(void);

/**
 * Make match byte array from the flow.
 *
 * @param[in]   flow    Flow.
 *
 */
void flow_make_match(struct flow *flow);

#endif /* SRC_INCLUDE_LAGOPUS_FLOWINFO_H_ */
