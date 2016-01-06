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

#ifndef __PTREE_H__
#define __PTREE_H__

#include <stdbool.h>

/* Patricia tree top structure. */
struct ptree {
  /* Top node. */
  struct ptree_node *top;

  /* Maximum key size allowed (in bits). */
  uint32_t max_key_len;

  /* Identifier of the FIB the tree is for - default is 0 */
  uint32_t id;
};

/* Patricia tree node structure. */
struct ptree_node {
  struct ptree_node *link[2];
#define  p_left      link[0]
#define  p_right     link[1]

  /* Tree link. */
  struct ptree *tree;
  struct ptree_node *parent;

  /* Lock of this radix. */
  uint32_t lock;

  /* Each node of route. */
  void *info;

  /* Key len (in bits). */
  uint32_t key_len;

  /* Key begins here. */
  uint8_t key [1];
};

#define PTREE_KEY_MIN_LEN       1
#define PTREE_NODE_KEY(n)       (& (n)->key [0])
#define PTREE_LOOP(T,V,N)                                                     \
  if (T)                                                                      \
    for ((N) = ptree_top ((T)); (N); (N) = ptree_next ((N)))                  \
      if (((V) = (N)->info) != NULL)

/* Prototypes. */
struct ptree *ptree_init(uint32_t max_key_len);
struct ptree_node *ptree_top(struct ptree *tree);
struct ptree_node *ptree_next(struct ptree_node *node);
struct ptree_node *ptree_next_until(struct ptree_node *node1,
                                    struct ptree_node *node2);
struct ptree_node *ptree_node_get(struct ptree *tree, uint8_t *key,
                                  uint32_t key_len);
struct ptree_node *ptree_node_lookup(struct ptree *tree, uint8_t *key,
                                     uint32_t key_len);
struct ptree_node *ptree_node_match1(struct ptree *tree, uint8_t *key,
                                     uint32_t key_len);
struct ptree_node *ptree_node_match(struct ptree *tree, uint8_t *key,
                                    uint32_t key_len);
struct ptree_node *ptree_lock_node(struct ptree_node *node);
void ptree_unlock_node(struct ptree_node *node);

void ptree_free(struct ptree *tree);
int ptree_has_info(struct ptree *tree);
void ptree_key_copy(struct ptree_node *node, uint8_t *key, uint32_t key_len);
bool ptree_key_match(uint8_t *np, uint32_t n_len, uint8_t *pp, uint32_t p_len);

#endif /* __PTREE_H__ */
