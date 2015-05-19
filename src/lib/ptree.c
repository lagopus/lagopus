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


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lagopus/ptree.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* Initialize tree. max_key_len is in bits. */
struct ptree *
ptree_init(uint32_t max_key_len) {
  struct ptree *tree;

  if (! max_key_len) {
    return NULL;
  }

  tree = calloc(1, sizeof(struct ptree));
  if (tree == NULL) {
    return NULL;
  }

  tree->max_key_len = max_key_len;

  return tree;
}

/* Free route node. */
static void
ptree_node_free(struct ptree_node *node) {
  free(node);
}

/* Free route tree. */
void
ptree_free(struct ptree *rt) {
  struct ptree_node *tmp_node;
  struct ptree_node *node;

  if (rt == NULL) {
    return;
  }

  node = rt->top;

  while (node) {
    if (node->p_left) {
      node = node->p_left;
      continue;
    }

    if (node->p_right) {
      node = node->p_right;
      continue;
    }

    tmp_node = node;
    node = node->parent;

    if (node != NULL) {
      if (node->p_left == tmp_node) {
        node->p_left = NULL;
      } else {
        node->p_right = NULL;
      }

      ptree_node_free(tmp_node);
    } else {
      ptree_node_free(tmp_node);
      break;
    }
  }

  free(rt);

  return;
}

/* Delete node from the routing tree. */
static void
ptree_node_delete(struct ptree_node *node) {
  struct ptree_node *child, *parent;

  assert(node->lock == 0);
  assert(node->info == NULL);

  if (node->p_left && node->p_right) {
    return;
  }

  if (node->p_left) {
    child = node->p_left;
  } else {
    child = node->p_right;
  }

  parent = node->parent;

  if (child) {
    child->parent = parent;
  }

  if (parent) {
    if (parent->p_left == node) {
      parent->p_left = child;
    } else {
      parent->p_right = child;
    }
  } else {
    node->tree->top = child;
  }

  ptree_node_free(node);

  /* If parent node is stub then delete it also. */
  if (parent && parent->lock == 0) {
    ptree_node_delete(parent);
  }
}

static size_t
ptree_bit_to_octets(uint32_t key_len) {
  return (size_t)(MAX((key_len + 7) / 8, PTREE_KEY_MIN_LEN));
}

/* Set key in node. */
void
ptree_key_copy(struct ptree_node *node, uint8_t *key, uint32_t key_len) {
  size_t octets;

  if (key_len == 0) {
    return;
  }

  octets = ptree_bit_to_octets(key_len);
  memcpy(PTREE_NODE_KEY(node), key, octets);
}

/* Allocate new route node. */
static struct ptree_node *
ptree_node_create(uint32_t key_len) {
  struct ptree_node *pn;
  size_t octets;

  octets = ptree_bit_to_octets(key_len);

  pn = calloc(1, sizeof(struct ptree_node) + octets);
  if (! pn) {
    return NULL;
  }

  pn->key_len = key_len;

  return pn;
}

/* Utility mask array. */
static const uint8_t maskbit[] = {
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/* Match keys. If n includes p prefix then return TRUE else return FALSE. */
bool
ptree_key_match(uint8_t *np, uint32_t n_len, uint8_t *pp, uint32_t p_len) {
  uint32_t offset;
  uint32_t shift;

  if (n_len > p_len) {
    return false;
  }

  offset =MIN(n_len, p_len) / 8;
  shift = MIN(n_len, p_len) % 8;

  if (shift) {
    if (maskbit[shift] & (np[offset] ^ pp[offset])) {
      return false;
    }
  }

  while (offset--) {
    if (np[offset] != pp[offset]) {
      return false;
    }
  }

  return true;
}

/* Match keys. If n includes p prefix then return TRUE else return FALSE.
 * This function supports search prefix to be smaller than the node prefix
 */
static bool
ptree_key_match1(uint8_t *np, uint32_t n_len, uint8_t *pp, uint32_t p_len) {
  uint32_t offset;
  uint32_t shift;

  offset = MIN(n_len, p_len) / 8;
  shift = MIN(n_len, p_len) % 8;

  if (shift) {
    if (maskbit[shift] & (np[offset] ^ pp[offset])) {
      return false;
    }
  }

  while (offset--) {
    if (np[offset] != pp[offset]) {
      return false;
    }
  }

  return true;
}

/* Allocate new route node with ptree_key set. */
static struct ptree_node *
ptree_node_set(struct ptree *tree, uint8_t *key, uint32_t key_len) {
  struct ptree_node *node;

  node = ptree_node_create(key_len);
  if (! node) {
    return NULL;
  }

  /* Copy over key. */
  ptree_key_copy(node, key, key_len);
  node->tree = tree;

  return node;
}

/* Common ptree_key route genaration. */
static struct ptree_node *
ptree_node_common(struct ptree_node *n, uint8_t *pp, uint32_t p_len) {
  uint32_t i;
  uint32_t j;
  uint8_t diff;
  uint8_t mask;
  uint32_t key_len;
  struct ptree_node *new;
  uint8_t *np;
  uint8_t *newp;
  uint8_t boundary = 0;

  np = PTREE_NODE_KEY(n);

  for (i = 0; i < p_len / 8; i++)
    if (np[i] != pp[i]) {
      break;
    }

  key_len = i * 8;

  if (key_len != p_len) {
    diff = np[i] ^ pp[i];
    mask = 0x80;
    while (key_len < p_len && !(mask & diff)) {
      if (boundary == 0) {
        boundary = 1;
      }
      mask >>= 1;
      key_len++;
    }
  }

  /* Fill new key. */
  new = ptree_node_create(key_len);
  if (! new) {
    return NULL;
  }

  newp = PTREE_NODE_KEY(new);

  for (j = 0; j < i; j++) {
    newp[j] = np[j];
  }

  if (boundary) {
    newp[j] = np[j] & maskbit[new->key_len % 8];
  }

  return new;
}

/* Check bit of the ptree_key. */
static int
ptree_check_bit(struct ptree *tree, uint8_t *p, uint32_t key_len) {
  uint32_t offset;
  uint32_t shift;

  assert(tree->max_key_len >= key_len);

  offset =key_len / 8;
  shift = 7 - (key_len % 8);

  return (p[offset] >> shift & 1);
}

static void
ptree_set_link(struct ptree_node *node, struct ptree_node *new) {
  int bit;

  bit = ptree_check_bit(new->tree, PTREE_NODE_KEY(new), node->key_len);

  assert(bit == 0 || bit == 1);

  node->link[bit] = new;
  new->parent = node;
}

/* Lock node. */
struct ptree_node *
ptree_lock_node(struct ptree_node *node) {
  node->lock++;
  return node;
}

/* Unlock node. */
void
ptree_unlock_node(struct ptree_node *node) {
  node->lock--;

#if 0
  if (node->lock == 0) {
    ptree_node_delete(node);
  }
#endif
}

/* Find matched ptree_key. This function supports search prefix len
 * less than the node prefix len. */
struct ptree_node *
ptree_node_match1(struct ptree *tree, uint8_t *key, uint32_t key_len) {
  struct ptree_node *node;
  struct ptree_node *matched;

  if (key_len > tree->max_key_len) {
    return NULL;
  }

  matched = NULL;
  node = tree->top;

  /* Walk down tree.  If there is matched route then store it to
     matched. */
  while (node && ptree_key_match1(PTREE_NODE_KEY(node),
                                  node->key_len, key, key_len)) {
    matched = node;
    node = node->link[ptree_check_bit(tree, key, node->key_len)];

    if (matched->key_len >= key_len) {
      break;
    }
  }

  /* If matched route found, return it. */
  if (matched) {
    return ptree_lock_node(matched);
  }

  return NULL;
}


/* Find matched ptree_key. */
struct ptree_node *
ptree_node_match(struct ptree *tree, uint8_t *key, uint32_t key_len) {
  struct ptree_node *node;
  struct ptree_node *matched;

  if (key_len > tree->max_key_len) {
    return NULL;
  }

  matched = NULL;
  node = tree->top;

  /* Walk down tree.  If there is matched route then store it to
     matched. */
  while (node && (node->key_len <= key_len)) {
    if (node->info == NULL) {
      node = node->link[ptree_check_bit(tree, key, node->key_len)];
      continue;
    }

    if (! ptree_key_match(PTREE_NODE_KEY(node), node->key_len, key, key_len)) {
      break;
    }

    matched = node;
    node = node->link[ptree_check_bit(tree, key, node->key_len)];
  }

  /* If matched route found, return it. */
  if (matched) {
    return ptree_lock_node(matched);
  }

  return NULL;
}

/* Lookup same ptree_key node.  Return NULL when we can't find the node. */
struct ptree_node *
ptree_node_lookup(struct ptree *tree, uint8_t *key, uint32_t key_len) {
  struct ptree_node *node;

  if (key_len > tree->max_key_len) {
    return NULL;
  }

  node = tree->top;
  while (node && node->key_len <= key_len &&
         ptree_key_match(PTREE_NODE_KEY(node), node->key_len, key, key_len)) {
    if (node->key_len == key_len && node->info) {
      return ptree_lock_node(node);
    }
    node = node->link[ptree_check_bit(tree, key, node->key_len)];
  }

  return NULL;
}

/* Add node to routing tree. */
struct ptree_node *
ptree_node_get(struct ptree *tree, uint8_t *key, uint32_t key_len) {
  struct ptree_node *new, *node, *match;

  if (key_len > tree->max_key_len) {
    return NULL;
  }

  match = NULL;
  node = tree->top;
  while (node && node->key_len <= key_len &&
         ptree_key_match(PTREE_NODE_KEY(node), node->key_len, key, key_len)) {
    if (node->key_len == key_len) {
      return ptree_lock_node(node);
    }

    match = node;
    node = node->link[ptree_check_bit(tree, key, node->key_len)];
  }

  if (node == NULL) {
    new = ptree_node_set(tree, key, key_len);
    if (match) {
      ptree_set_link(match, new);
    } else {
      tree->top = new;
    }
  } else {
    new = ptree_node_common(node, key, key_len);
    if (! new) {
      return NULL;
    }
    new->tree = tree;
    ptree_set_link(new, node);

    if (match) {
      ptree_set_link(match, new);
    } else {
      tree->top = new;
    }

    if (new->key_len != key_len) {
      match = new;
      new = ptree_node_set(tree, key, key_len);
      ptree_set_link(match, new);
    }
  }

  ptree_lock_node(new);
  return new;
}

/* Get fist node and lock it.  This function is useful when one want
   to lookup all the node exist in the routing tree. */
struct ptree_node *
ptree_top(struct ptree *tree) {
  /* If there is no node in the routing tree return NULL. */
  if (tree == NULL || tree->top == NULL) {
    return NULL;
  }

  /* Lock the top node and return it. */
  return ptree_lock_node(tree->top);
}

/* Unlock current node and lock next node then return it. */
struct ptree_node *
ptree_next(struct ptree_node *node) {
  struct ptree_node *next, *start;

  /* Node may be deleted from ptree_unlock_node so we have to preserve
     next node's pointer. */
  if (node->p_left) {
    next = node->p_left;
    ptree_lock_node(next);
    ptree_unlock_node(node);
    return next;
  }
  if (node->p_right) {
    next = node->p_right;
    ptree_lock_node(next);
    ptree_unlock_node(node);
    return next;
  }

  start = node;
  while (node->parent) {
    if (node->parent->p_left == node && node->parent->p_right) {
      next = node->parent->p_right;
      ptree_lock_node(next);
      ptree_unlock_node(start);
      return next;
    }
    node = node->parent;
  }
  ptree_unlock_node(start);
  return NULL;
}

/* Unlock current node and lock next node until limit. */
struct ptree_node *
ptree_next_until(struct ptree_node *node, struct ptree_node *limit) {
  struct ptree_node *next, *start;

  /* Node may be deleted from ptree_unlock_node so we have to preserve
     next node's pointer. */

  if (node->p_left) {
    next = node->p_left;
    ptree_lock_node(next);
    ptree_unlock_node(node);
    return next;
  }
  if (node->p_right) {
    next = node->p_right;
    ptree_lock_node(next);
    ptree_unlock_node(node);
    return next;
  }

  start = node;
  while (node->parent && node != limit) {
    if (node->parent->p_left == node && node->parent->p_right) {
      next = node->parent->p_right;
      ptree_lock_node(next);
      ptree_unlock_node(start);
      return next;
    }
    node = node->parent;
  }
  ptree_unlock_node(start);
  return NULL;
}

/* Check if the tree contains nodes with info set. */
int
ptree_has_info(struct ptree *tree) {
  struct ptree_node *node;

  if (tree == NULL) {
    return 0;
  }

  node = tree->top;
  while (node) {
    if (node->info) {
      return 1;
    }

    if (node->p_left) {
      node = node->p_left;
      continue;
    }

    if (node->p_right) {
      node = node->p_right;
      continue;
    }

    while (node->parent) {
      if (node->parent->p_left == node && node->parent->p_right) {
        node = node->parent->p_right;
        break;
      }
      node = node->parent;
    }

    if (node->parent == NULL) {
      break;
    }
  }

  return 0;
}
