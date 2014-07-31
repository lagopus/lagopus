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


#ifndef SRC_CONFIG_CNODE_H_
#define SRC_CONFIG_CNODE_H_

/* Library include. */
#include "lagopus/vector.h"

/* Forward declaration. */
struct confsys;
struct cstruct;
enum match_type;

/* Types of cnode. */
enum cnode_type {
  CNODE_TYPE_KEYWORD,
  CNODE_TYPE_WORD,
  CNODE_TYPE_RANGE,
  CNODE_TYPE_IPV4,
  CNODE_TYPE_IPV4_PREFIX,
  CNODE_TYPE_IPV6,
  CNODE_TYPE_IPV6_PREFIX,
  CNODE_TYPE_MAC
};

/* Flags of cnode. */
enum cnode_flags {
  CNODE_FLAG_LEAF        = (1 << 0),
  CNODE_FLAG_MULTI       = (1 << 1),
  CNODE_FLAG_VALUE       = (1 << 2),
  CNODE_FLAG_SET_NODE    = (1 << 3),
  CNODE_FLAG_DELETE_NODE = (1 << 4),
  CNODE_FLAG_DEFAULT     = (1 << 5)
};

struct confsys;
typedef int (*cnode_callback_t)(struct confsys *, int, const char **);

/* Config node. */
struct cnode {
  /* Name of the cnode. */
  char *name;

  /* Printable name of the cnode. */
  char *guide;

  /* Help of the cnode. */
  char *help;

  /* Range string of the cnode. */
  char *range;

  /* Default value of the cnode. */
  char *default_value;

  /* Type of the cnode. */
  enum cnode_type type;

  /* Flags of the cnode. */
  uint32_t flags;

  /* Range. */
  int64_t min;
  int64_t max;

  /* Priority. */
  int priority;

  /* Parent cnode. */
  struct cnode *parent;

  /* Child cnode vector. */
  struct vector *v;

  /* Command node. */
  cnode_callback_t callback;
};

#define CHECK_FLAG(V, F)       ((V) & (F))
#define SET32_FLAG(V, F)       (V) = (V) | (uint32_t)(F)
#define UNSET32_FLAG(V, F)     (V) = (V) & (uint32_t)~(F)

struct cnode *
cnode_alloc(void);

void
cnode_free(struct cnode *cnode);

void
cnode_child_add(struct cnode *parent, struct cnode *child);

void
cnode_child_delete(struct cnode *parent, struct cnode *child);

void
cnode_child_free(struct cnode *cnode);

void
cnode_sort(struct cnode *cnode);

void
cnode_type_set(struct cnode *cnode);

struct cnode *
cnode_lookup(struct cnode *cnode, const char *name);

struct cnode *
cnode_match(struct cnode *cnode, char *name);

void
cnode_schema_match(struct cnode *schema, char *name,
                   enum match_type *match_type);

void
cnode_replace_name(struct cnode *cnode, char *name);

int
cnode_is_leaf(struct cnode *cnode);

int
cnode_is_multi(struct cnode *cnode);

int
cnode_is_value(struct cnode *cnode);

int
cnode_is_top(struct cnode *cnode);

int
cnode_is_keyword(struct cnode *cnode);

int
cnode_has_default(struct cnode *cnode);

void
cnode_set_flag(struct cnode *cnode, int flag);

/**
 *  Copy same child node from src node to dst node.
 *
 * @param[in]	top	Top node which src and dst node belongs to.
 * @param[in]	src	Source node to be copied.
 * @param[in]	dst	Destination node.
 */
void
cnode_link(struct cnode *top, const char *src, const char *dst);

#endif /* SRC_CONFIG_CNODE_H_ */
