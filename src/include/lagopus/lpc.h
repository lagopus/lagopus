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

#ifndef __LPC_H__
#define __LPC_H__

#include <stdint.h>
#include "qlist.h"

/* LPC key vector. */
struct lpc_vector {
  /* 32bit IPv4 prefix key in host byte order. */
  uint32_t key;

  /* Bit comparison point.  Value range is 31 - 0. */
  u_char pos;

  /* Bit length to be compared. */
  u_char bits;

  /* Host length. */
  u_char slen;

  union {
    /* Leaf list (pos | bits)  == 0. */
    struct qlist_head leaf;

    /* Node vectors (pos | bits) > 0. */
    struct lpc_vector *node[0];
  };
};

/* LPC Node. */
struct lpc_node {
  /* Number of empty children. */
  uint32_t empty_children;

  /* Number of full children. */
  uint32_t full_children;

  /* Pointer to parent node. */
  struct lpc_vector *parent;

  /* Pointer to child node. */
  struct lpc_vector lv[1];
};

/* LPC top node structure. */
struct lpc {
  /* Top vector node. */
  struct lpc_vector lv[1];
};

/* LPC information node which has prefix information and nexthop. */
struct lpc_info {
  /* Linked list of lpc_info. */
  struct qlist_node	qlist;

  /* Host bits.  */
  u_char slen;

  /* Nexthop information (structure is defined by caller).  */
  void *nexthop;
};


struct lpc *
lpc_init(void);

void
lpc_free(struct lpc *lpc);

int
lpc_table_insert(struct lpc *t, struct in_addr addr, u_char plen);

int
lpc_table_delete(struct lpc *t, struct in_addr addr, u_char plen);

int
lpc_table_lookup(struct lpc *t, struct in_addr *daddr,
                 struct lpc_vector **lv, u_char *plen);

void
lpc_table_dump(struct lpc_vector *lv);

#endif /* __LPC_H__ */
