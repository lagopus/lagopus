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

#ifndef __QLIST_H__
#define __QLIST_H__

#include <stdio.h>

struct qlist_head {
  struct qlist_node *first;
};

struct qlist_node {
  struct qlist_node *next, **pprev;
};

static inline void
qlist_init_head(struct qlist_head *h)
{
  h->first = NULL;
}

static inline void
qlist_init_node(struct qlist_node *h)
{
  h->next = NULL;
  h->pprev = NULL;
}

static inline void
qlist_add_head(struct qlist_node *n, struct qlist_head *h)
{
  struct qlist_node *first = h->first;
  n->next = first;
  if (first)
    first->pprev = &n->next;
  h->first = n;
  n->pprev = &h->first;
}

static inline void
qlist_add_before(struct qlist_node *n,
                 struct qlist_node *next)
{
  n->pprev = next->pprev;
  n->next = next;
  next->pprev = &n->next;
  *(n->pprev) = n;
}

static inline void
qlist_add_behind(struct qlist_node *n, struct qlist_node *prev)
{
  n->next = prev->next;
  prev->next = n;
  n->pprev = &prev->next;
  
  if (n->next)
    n->next->pprev  = &n->next;
}

static inline void
qlist_del(struct qlist_node *n)
{
  struct qlist_node *next = n->next;
  struct qlist_node **pprev = n->pprev;
  *pprev = next;
  if (next)
    next->pprev = pprev;
  n->next = NULL;
  n->pprev = NULL;
}

static inline int
qlist_empty(const struct qlist_head *h)
{
  return !h->first;
}

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({              \
      (type *)((char *)ptr - offsetof(type,member) );})

#define qlist_entry(ptr, type, member) container_of(ptr,type,member)

#define qlist_entry_safe(ptr, type, member)                             \
  ({ typeof(ptr) ____ptr = (ptr);                                       \
    ____ptr ? qlist_entry(____ptr, type, member) : NULL;                \
  })

#define qlist_for_each_entry(pos, head, member)                         \
  for (pos = qlist_entry_safe((head)->first, typeof(*(pos)), member);   \
       pos;                                                             \
       pos = qlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define qlist_for_each_entry_from(pos, member)				\
  for (; pos;                                                           \
       pos = qlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define qlist_reverse_each_entry(pos, prev, head, member)               \
  for (prev = pos;                                                      \
       pos && (prev == pos || (prev)->member.pprev != &(head)->first);  \
       prev = pos,                                                      \
       pos = qlist_entry_safe((pos)->member.pprev, typeof(*(pos)), member))

#endif /* __QLIST_H__ */
