/* %COPYRIGHT% */

#ifndef __LPC_H__
#define __LPC_H__

#include <stdio.h>
#include <stdint.h>

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
