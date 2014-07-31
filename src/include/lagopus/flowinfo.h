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
 *	@file	flowinfo.h
 *	@brief	Flow information for fast lookup.
 */

#ifndef _SRC_INCLUDE_LAGOPUS_FLOWINFO_H
#define _SRC_INCLUDE_LAGOPUS_FLOWINFO_H

/**
 * definition of structured flow table.
 */
struct flowinfo {
  int nflow;			/*< number of entries. */
  int nnext;			/*< number of child flowinfo. */
  union {			/*< entries includes type specific match. */
    struct ptree *ptree;	/*< patricia tree entries. */
    struct flow **flows;	/*< simple array entries. */
    struct flowinfo **next;	/*< child flowinfo array. */
    /* add more types if needed. */
  };
  struct flowinfo *misc;	/*< flowinfo includes no specific match. */
  uint64_t userdata;		/*< type specific data placeholder. */

  lagopus_result_t (*add_func)(struct flowinfo *, struct flow *);
  struct flow *(*match_func)(struct flowinfo *, struct lagopus_packet *,
                             int32_t *);
  lagopus_result_t (*del_func)(struct flowinfo *, struct flow *);
  struct flow *(*find_func)(struct flowinfo *, struct flow *);
  void (*destroy_func)(struct flowinfo *);
};

struct flowinfo *new_flowinfo_basic(void);
struct flowinfo *new_flowinfo_in_port(void);
struct flowinfo *new_flowinfo_metadata(void);
struct flowinfo *new_flowinfo_vlan_vid(void);
struct flowinfo *new_flowinfo_eth_type(void);
struct flowinfo *new_flowinfo_ipv4_dst_mask(void);
struct flowinfo *new_flowinfo_ipv4_dst(void);
struct flowinfo *new_flowinfo_ipv4(void);
struct flowinfo *new_flowinfo_ipv6(void);
struct flowinfo *new_flowinfo_mpls(void);
struct flowinfo *new_flowinfo_metadata(void);
struct flowinfo *new_flowinfo_metadata_mask(void);

void flowinfo_init(void);

#endif /* _SRC_INCLUDE_LAGOPUS_FLOWINFO_H */
