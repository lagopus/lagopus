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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cnode.h"
#include "validate.h"

struct cnode *
cnode_alloc(void) {
  struct cnode *cnode;

  cnode = (struct cnode *)calloc(1, sizeof(struct cnode));
  if (cnode == NULL) {
    return NULL;
  }

  cnode->v = vector_alloc();
  if (cnode->v == NULL) {
    cnode_free(cnode);
    return NULL;
  }

  return cnode;
}

void
cnode_free(struct cnode *cnode) {
  if (cnode->name != NULL) {
    free(cnode->name);
  }
  if (cnode->guide != NULL) {
    free(cnode->guide);
  }
  if (cnode->range != NULL) {
    free(cnode->range);
  }
  if (cnode->help != NULL) {
    free(cnode->help);
  }
  if (cnode->default_value != NULL) {
    free(cnode->default_value);
  }
  if (cnode->v != NULL) {
    vector_free(cnode->v);
  }
  free(cnode);
}

void
cnode_child_add(struct cnode *parent, struct cnode *child) {
  vector_set(parent->v, child);
  child->parent = parent;
}

void
cnode_child_delete(struct cnode *parent, struct cnode *child) {
  vector_unset(parent->v, child);
}

void
cnode_child_free(struct cnode *cnode) {
  vindex_t i;
  struct cnode *child;

  for (i = 0; i < vector_max(cnode->v); i++) {
    if ((child = vector_slot(cnode->v, i)) != NULL) {
      cnode_child_free(child);
    }
  }
  cnode_free(cnode);
}

static int
cnode_cmp_func(const void *p, const void *q) {
  struct cnode *a = *(struct cnode **)p;
  struct cnode *b = *(struct cnode **)q;

  if (a->priority != b->priority) {
    return b->priority - a->priority;
  }
  return strcmp(a->name, b->name);
}

void
cnode_sort(struct cnode *cnode) {
  struct vector *v;
  vindex_t i;

  v = cnode->v;
  qsort(v->index, v->max, sizeof(void *), cnode_cmp_func);

  for (i = 0; i < vector_max(v); i++) {
    struct cnode *cn = (struct cnode *)vector_slot(v, i);
    cnode_sort(cn);
  }
}

static void
cnode_range_set(struct cnode *cnode) {
  char *p;
  char *cp;
  int64_t min = 0;
  int64_t max = 0;
  char *range;

  if (cnode->range != NULL) {
    range = cnode->range;
  } else {
    range = cnode->name;
  }

  p = strrchr(range, '-');
  if (p == NULL) {
    return;
  }

  *p = '\0';
  cp = str2int64(range, &min);
  if (!cp) {
    return;
  }
  *p = '-';

  range = p + 1;
  cp = str2int64(range, &max);
  if (!cp) {
    return;
  }

  cnode->min = min;
  cnode->max = max;
}

void
cnode_type_set(struct cnode *cnode) {
  if (strlen(cnode->name) == 0) {
    cnode->type = CNODE_TYPE_KEYWORD;
  } else if (strcmp(cnode->name, "WORD") == 0) {
    cnode->type = CNODE_TYPE_WORD;
  } else if (strcmp(cnode->name, "A.B.C.D") == 0) {
    cnode->type = CNODE_TYPE_IPV4;
  } else if (strcmp(cnode->name, "A.B.C.D/M") == 0) {
    cnode->type = CNODE_TYPE_IPV4_PREFIX;
  } else if (strcmp(cnode->name, "X:X::X:X") == 0) {
    cnode->type = CNODE_TYPE_IPV6;
  } else if (strcmp(cnode->name, "X:X::X:X/M") == 0) {
    cnode->type = CNODE_TYPE_IPV6_PREFIX;
  } else if (strcmp(cnode->name, "XX:XX:XX:XX:XX:XX") == 0) {
    cnode->type = CNODE_TYPE_MAC;
  } else if (strcmp(cnode->name, "INTEGER") == 0 ||
             isdigit(cnode->name[0]) || cnode->name[0] == '-') {
    cnode->type = CNODE_TYPE_RANGE;
    cnode_range_set(cnode);
  } else {
    cnode->type = CNODE_TYPE_KEYWORD;
  }
}

struct cnode *
cnode_lookup(struct cnode *cnode, const char *name) {
  uint32_t i;
  struct cnode *cn;

  if (cnode == NULL) {
    return NULL;
  }

  for (i = 0; i < vector_max(cnode->v); i++) {
    if ((cn = vector_slot(cnode->v, i)) != NULL) {
      if (strcmp(cn->name, name) == 0) {
        return cn;
      }
    }
  }
  return NULL;
}

/* Perform name match against the schema.  Result is stored in
 * match_type. */
void
cnode_schema_match(struct cnode *schema, char *name,
                   enum match_type *match_type) {
  switch (schema->type) {
    case CNODE_TYPE_KEYWORD:
      keyword_match(name, schema->name, match_type);
      break;
    case CNODE_TYPE_WORD:
      word_match(name, match_type);
      break;
    case CNODE_TYPE_IPV4:
      ipv4_match(name, match_type);
      break;
    case CNODE_TYPE_IPV4_PREFIX:
      ipv4_prefix_match(name, match_type);
      break;
    case CNODE_TYPE_IPV6:
      ipv6_match(name, match_type);
      break;
    case CNODE_TYPE_IPV6_PREFIX:
      ipv6_prefix_match(name, match_type);
      break;
    case CNODE_TYPE_MAC:
      mac_address_match(name, match_type);
      break;
    case CNODE_TYPE_RANGE:
      range_match(name, schema, match_type);
      break;
    default:
      *match_type = NONE_MATCH;
      break;
  }
}

/* Replace current cnode name. */
void
cnode_replace_name(struct cnode *cnode, char *name) {
  if (cnode->name) {
    free(cnode->name);
  }
  cnode->name = strdup(name);
}

struct cnode *
cnode_match(struct cnode *cnode, char *name) {
  uint32_t i;
  struct cnode *child;
  enum match_type type;

  if (cnode == NULL) {
    return NULL;
  }

  for (i = 0; i < vector_max(cnode->v); i++) {
    if ((child = vector_slot(cnode->v, i)) != NULL) {
      switch (child->type) {
        case CNODE_TYPE_KEYWORD:
          if (strcmp(child->name, name) == 0) {
            return child;
          }
          break;
        case CNODE_TYPE_WORD:
          return child;
          break;
        case CNODE_TYPE_IPV4:
          ipv4_match(name, &type);
          if (type == IPV4_MATCH) {
            return child;
          }
          break;
        case CNODE_TYPE_IPV4_PREFIX:
          ipv4_prefix_match(name, &type);
          if (type == IPV4_PREFIX_MATCH) {
            return child;
          }
          break;
        case CNODE_TYPE_IPV6:
          ipv6_match(name, &type);
          if (type == IPV6_MATCH) {
            return child;
          }
          break;
        case CNODE_TYPE_IPV6_PREFIX:
          ipv6_prefix_match(name, &type);
          if (type == IPV6_PREFIX_MATCH) {
            return child;
          }
          break;
        case CNODE_TYPE_MAC:
          mac_address_match(name, &type);
          if (type == MAC_ADDRESS_MATCH) {
            return child;
          }
          break;
        case CNODE_TYPE_RANGE:
          range_match(name, child, &type);
          if (type == RANGE_MATCH) {
            return child;
          }
          break;
        default:
          break;
      }
    }
  }
  return NULL;
}

int
cnode_is_leaf(struct cnode *cnode) {
  if (CHECK_FLAG(cnode->flags, CNODE_FLAG_LEAF) ||
      vector_max(cnode->v) == 0) {
    return 1;
  } else {
    return 0;
  }
}

int
cnode_is_multi(struct cnode *cnode) {
  if (CHECK_FLAG(cnode->flags, CNODE_FLAG_MULTI)) {
    return 1;
  } else {
    return 0;
  }
}

int
cnode_is_value(struct cnode *cnode) {
  if (CHECK_FLAG(cnode->flags, CNODE_FLAG_VALUE)) {
    return 1;
  } else {
    return 0;
  }
}

int
cnode_is_top(struct cnode *cnode) {
  if (cnode->parent) {
    return 0;
  } else {
    return 1;
  }
}

int
cnode_is_keyword(struct cnode *cnode) {
  if (cnode->name && islower(cnode->name[0])) {
    return 1;
  } else {
    return 0;
  }
}

int
cnode_has_default(struct cnode *cnode) {
  if (CHECK_FLAG(cnode->flags, CNODE_FLAG_DEFAULT)) {
    return 1;
  } else {
    return 0;
  }
}

void
cnode_set_flag(struct cnode *cnode, int flag) {
  SET32_FLAG(cnode->flags, flag);
}

void
cnode_link(struct cnode *top, const char *src, const char *dst) {
  struct cnode *snode;
  struct cnode *dnode;

  /* Lookup source node. */
  snode = cnode_lookup(top, src);
  if (!snode) {
    return;
  }

  /* Create destination node. */
  dnode = cnode_alloc();
  dnode->name = strdup(dst);
  cnode_child_add(top, dnode);

  /* Append src child to dst child. */
  vector_append(dnode->v, snode->v);
}
