/* %COPYRIGHT% */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lagopus_apis.h"
#include "lpc.h"


#define INFLATE_THRESHOLD 50
#define INFLATE_THRESHOLD_ROOT 30
#define HALVE_THRESHOLD  25
#define HALVE_THRESHOLD_ROOT 15

static int lpc_node_alloc = 0;
static int lpc_info_alloc = 0;

/* IPv4 prefix length. */
#define KEYLENGTH	        32
#define KEY_MAX                 ((uint32_t)~0)

/* Macro to determine node type. */
#define IS_TOP(n)	        ((n)->pos == KEYLENGTH)
#define IS_NODE(n)	        ((n)->bits)
#define IS_LEAF(n)	        (!(n)->bits)

/* Macros for offset handling. */
#define node_info(x)            container_of(x, struct lpc_node, lv[0])
#define node_parent(tn)         node_info(tn)->parent
#define node_init_parent(n, p)  node_info(n)->parent = p
#define node_size(n)	        offsetof(struct lpc_node, lv[0].node[n])

/* Macros for child handling. */
#define get_child(tn, i)        (tn)->node[i]
#define get_cindex(key, lv)     (((key) ^ (lv)->key) >> (lv)->pos)
#define child_length(lv)        ((1ul << lv->bits) & ~(1ul))

/* Find last bit.  */
static inline unsigned long
__fls(unsigned long word)
{
  asm("bsr %1,%0" : "=r"(word) : "rm"(word));
  return word;
}

/* Utility function to get the comparison bit index. */
static inline unsigned long
get_index(uint32_t key, struct lpc_vector *lv)
{
  unsigned long index = key ^ lv->key;
  
  if (KEYLENGTH == lv->pos) {
    return 0;
  }

  return index >> lv->pos;
}

/* Allocate a new LPC node with the bits.  */
static struct lpc_vector *
lpc_node_new(uint32_t key, u_char pos, u_char bits)
{
  struct lpc_node *node;
  struct lpc_vector *lv;
  unsigned int shift;
  
  node = (struct lpc_node *)calloc(node_size(1ul << bits), 1);
  if (! node)
    return NULL;
  
  lpc_node_alloc++;

  shift = (unsigned int)(pos + bits);
  lv = node->lv;

  if (bits == KEYLENGTH)
    node->full_children = 1;
  else
    node->empty_children = (uint32_t)(1ul << bits);
  
  /* Initialize key vector */
  lv->key = (shift < KEYLENGTH) ? (key >> shift) << shift : 0;
  lv->pos = pos;
  lv->bits = bits;
  lv->slen = pos;

  return lv;
}

/* Allocate a new LPC node as a leaf.  */
static struct lpc_vector *
lpc_leaf_new(uint32_t key, struct lpc_info *li)
{
  struct lpc_node *node;
  struct lpc_vector *lv;

  node = (struct lpc_node *)calloc(sizeof(struct lpc_node), 1);
  if (! node)
    return NULL;

  lpc_node_alloc++;

  lv = node->lv;

  /* Initialize key vector. */
  lv->key = key;
  lv->pos = 0;
  lv->bits = 0;
  lv->slen = li->slen;

  /* Link leaf to lpc info. */
  qlist_init_head(&lv->leaf);
  qlist_add_head(&li->qlist, &lv->leaf);

  return lv;
}

/* Free LPC node. */
static void
lpc_node_free(struct lpc_vector *lv)
{
  struct lpc_node *node = node_info(lv);
  lpc_node_alloc--;
  free(node);
}

static inline uint32_t
lpc_prefix_mismatch(uint32_t key, struct lpc_vector *lv)
{
  uint32_t prefix = lv->key;
  return (key ^ prefix) & (prefix | -prefix);
}

static inline void
lpc_node_set_parent(struct lpc_vector *lv, struct lpc_vector *lp)
{
  if (lv) {
    node_info(lv)->parent = lp;
  }
}

static inline int
lpc_node_full(struct lpc_vector *tn, struct lpc_vector *n)
{
  return n && ((n->pos + n->bits) == tn->pos) && IS_NODE(n);
}

/* Add a child at position i with overwrite the old value.  Update the
   value of full_children and empty_children. */
static void
lpc_put_child(struct lpc_vector *tn, unsigned long i, struct lpc_vector *n)
{
  struct lpc_vector *chi = get_child(tn, i);
  int isfull, wasfull;

  /* Update emptyChildren, overflow into fullChildren. */
  if (! n && chi) {
    if (! ++node_info(tn)->empty_children) {
      ++node_info(tn)->full_children;
    }
  }
  if (n && !chi) {
    if (! node_info(tn)->empty_children--) {
      node_info(tn)->full_children--;
    }
  }

  /* Update fullChildren. */
  wasfull = lpc_node_full(tn, chi);
  isfull = lpc_node_full(tn, n);
  
  if (wasfull && !isfull)
    node_info(tn)->full_children--;
  else if (!wasfull && isfull)
    node_info(tn)->full_children++;

  if (n && (tn->slen < n->slen))
    tn->slen = n->slen;

  tn->node[i] = n;
}

static inline void
lpc_put_child_root(struct lpc_vector *lp, uint32_t key, struct lpc_vector *n)
{
  if (IS_TOP(lp)) {
    lp->node[0] = n;
  } else {
    lpc_put_child(lp, get_index(key, lp), n);
  }
}

static unsigned char
lpc_update_suffix(struct lpc_vector *tn)
{
  unsigned char slen = tn->pos;
  unsigned long stride, i;

  for (i = 0, stride = 0x2ul ; i < child_length(tn); i += stride) {
    struct lpc_vector *n = get_child(tn, i);

    if (! n || (n->slen <= slen))
      continue;

    stride <<= (n->slen - slen);
    slen = n->slen;
    i &= ~(stride - 1);

    if ((slen + 1) >= (tn->pos + tn->bits))
      break;
  }

  tn->slen = slen;

  return slen;
}

static void
lpc_update_children(struct lpc_vector *tn)
{
  unsigned long i;
  
  for (i = child_length(tn); i;) {
    struct lpc_vector *inode = get_child(tn, --i);
    
    if (! inode)
      continue;
    
    if (node_parent(inode) == tn)
      lpc_update_children(inode);
    else
      lpc_node_set_parent(inode, tn);
  }
}

static void
lpc_leaf_push_suffix(struct lpc_vector *tn, struct lpc_vector *l)
{
  while (tn->slen < l->slen) {
    tn->slen = l->slen;
    tn = node_parent(tn);
  }
}

static void
lpc_leaf_pull_suffix(struct lpc_vector *lp, struct lpc_vector *l)
{
  while ((lp->slen > lp->pos) && (lp->slen > l->slen)) {
    if (lpc_update_suffix(lp) > l->slen)
      break;
    lp = node_parent(lp);
  }
}

static struct lpc_vector *
lpc_resize(struct lpc *t, struct lpc_vector *tn);

static inline int
lpc_should_inflate(struct lpc_vector *lp, struct lpc_vector *tn)
{
  unsigned long used = child_length(tn);
  unsigned long threshold = used;

  /* Keep root node larger. */
  threshold *= IS_TOP(lp) ? INFLATE_THRESHOLD_ROOT : INFLATE_THRESHOLD;
  used -= node_info(tn)->empty_children;
  used += node_info(tn)->full_children;
  
  return (used > 1) && tn->pos && ((50 * used) >= threshold);
}

static struct lpc_vector *
lpc_replace(struct lpc *t, struct lpc_vector *oldnode, struct lpc_vector *tn)
{
  struct lpc_vector *lp = node_parent(oldnode);
  unsigned long i;
  
  /* Setup the parent pointer out of and back into this node */
  node_init_parent(tn, lp);
  lpc_put_child_root(lp, tn->key, tn);
  
  /* Update all of the child parent pointers */
  lpc_update_children(tn);
  
  /* All pointers should be clean so we are done */
  lpc_node_free(oldnode);
  
  /* Resize children now that oldnode is freed */
  for (i = child_length(tn); i;) {
    struct lpc_vector *inode = get_child(tn, --i);
    
    /* Resize child node */
    if (lpc_node_full(tn, inode))
      tn = lpc_resize(t, inode);
  }
  
  return lp;
}

static struct lpc_vector *
lpc_inflate(struct lpc *t, struct lpc_vector *oldnode)
{
  struct lpc_vector *tn;
  unsigned long i;
  uint32_t m;

  tn = lpc_node_new(oldnode->key, (u_char)(oldnode->pos - 1),
                    (u_char)(oldnode->bits + 1));
  if (! tn)
    goto nonode;

  for (i = child_length(oldnode), m = 1u << tn->pos; i;) {
    struct lpc_vector *inode = get_child(oldnode, --i);
    struct lpc_vector *node0, *node1;
    unsigned long j, k;

    /* An empty child */
    if (! inode)
      continue;

    /* A leaf or an internal node with skipped bits */
    if (! lpc_node_full(oldnode, inode)) {
      lpc_put_child(tn, get_index(inode->key, tn), inode);
      continue;
    }

    /* An internal node with two children */
    if (inode->bits == 1) {
      lpc_put_child(tn, 2 * i + 1, get_child(inode, 1));
      lpc_put_child(tn, 2 * i, get_child(inode, 0));
      continue;
    }

    node1 = lpc_node_new(inode->key | m, inode->pos,
                         (u_char)(inode->bits - 1));
    if (! node1)
      goto nomem;

    node0 = lpc_node_new(inode->key, inode->pos,
                         (u_char)(inode->bits - 1));
    if (! node0)
      goto nomem;

    /* Populate child pointers in new nodes */
    for (k = child_length(inode), j = k / 2; j;) {
      lpc_put_child(node1, --j, get_child(inode, --k));
      lpc_put_child(node0, j, get_child(inode, j));
      lpc_put_child(node1, --j, get_child(inode, --k));
      lpc_put_child(node0, j, get_child(inode, j));
    }

    /* Link new nodes to parent */
    node_init_parent(node1, tn);
    node_init_parent(node0, tn);

    /* Link parent to nodes */
    lpc_put_child(tn, 2 * i + 1, node1);
    lpc_put_child(tn, 2 * i, node0);
  }

  /* Setup the parent pointers into and out of this node */
  return lpc_replace(t, oldnode, tn);

nomem:
  /* All pointers should be clean so we are done */
  lpc_node_free(tn);

nonode:
  return NULL;
}

static struct lpc_vector *
lpc_halve(struct lpc *t, struct lpc_vector *oldnode)
{
  struct lpc_vector *tn;
  unsigned long i;
  
  tn = lpc_node_new(oldnode->key,
                    (u_char)(oldnode->pos + 1),
                    (u_char)(oldnode->bits - 1));
  if (!tn)
    goto nonode;
  
  for (i = child_length(oldnode); i;) {
    struct lpc_vector *node1 = get_child(oldnode, --i);
    struct lpc_vector *node0 = get_child(oldnode, --i);
    struct lpc_vector *inode;
    
    /* At least one of the children is empty */
    if (!node1 || !node0) {
      lpc_put_child(tn, i / 2, node1 ? : node0);
      continue;
    }
    
    /* Two nonempty children */
    inode = lpc_node_new(node0->key, oldnode->pos, 1);
    if (!inode)
      goto nomem;
    
    /* Initialize pointers out of node */
    lpc_put_child(inode, 1, node1);
    lpc_put_child(inode, 0, node0);
    node_init_parent(inode, tn);
    
    /* Link parent to node */
    lpc_put_child(tn, i / 2, inode);
  }
  
  /* Setup the parent pointers into and out of this node */
  return lpc_replace(t, oldnode, tn);

nomem:
  /* All pointers should be clean so we are done */
  lpc_node_free(tn);

nonode:
  return NULL;
}

static inline int
lpc_should_halve(struct lpc_vector *lp, struct lpc_vector *tn)
{
  unsigned long used = child_length(tn);
  unsigned long threshold = used;
  
  threshold *= IS_TOP(lp) ? HALVE_THRESHOLD_ROOT : HALVE_THRESHOLD;
  used -= node_info(tn)->empty_children;
  
  return (used > 1) && (tn->bits > 1) && ((100 * used) < threshold);
}

static inline int
lpc_should_collapse(struct lpc_vector *tn)
{
  unsigned long used = child_length(tn);
  
  used -= node_info(tn)->empty_children;
  
  if ((tn->bits == KEYLENGTH) && node_info(tn)->full_children)
    used -= KEY_MAX;
  
  return used < 2;
}

static struct lpc_vector *
lpc_collapse(struct lpc_vector *oldnode)
{
  struct lpc_vector *n, *lp;
  unsigned long i;
  
  for (n = NULL, i = child_length(oldnode); !n && i;)
    n = get_child(oldnode, --i);
  
  lp = node_parent(oldnode);
  lpc_put_child_root(lp, oldnode->key, n);
  lpc_node_set_parent(n, lp);
  lpc_node_free(oldnode);
  
  return lp;
}

static struct lpc_vector *
lpc_resize(struct lpc *t, struct lpc_vector *tn)
{
  struct lpc_vector *lp = node_parent(tn);
  unsigned long cindex = get_index(tn->key, lp);
#define MAX_WORK 10
  int max_work = MAX_WORK;

  while (lpc_should_inflate(lp, tn) && max_work) {
    lp = lpc_inflate(t, tn);
    if (!lp) {
      break;
    }

    max_work--;
    tn = get_child(lp, cindex);
  }

  lp = node_parent(tn);

  if (max_work != MAX_WORK)
    return lp;

  while (lpc_should_halve(lp, tn) && max_work) {
    lp = lpc_halve(t, tn);
    if (!lp) {
      break;
    }

    max_work--;
    tn = get_child(lp, cindex);
  }

  /* Only one child remains */
  if (lpc_should_collapse(tn))
    return lpc_collapse(tn);

  /* Update parent in case halve failed */
  lp = node_parent(tn);

  /* Return if at least one deflate was run */
  if (max_work != MAX_WORK)
    return lp;

  /* Push the suffix length to the parent node */
  if (tn->slen > tn->pos) {
    unsigned char slen = lpc_update_suffix(tn);

    if (slen > lp->slen)
      lp->slen = slen;
  }
  return lp;
}

static void
lpc_rebalance(struct lpc *t, struct lpc_vector *tn)
{
  while (!IS_TOP(tn)) {
    tn = lpc_resize(t, tn);
  }
}

static int
lpc_insert_node(struct lpc *t, struct lpc_vector *lp,
                struct lpc_info *new, uint32_t key)
{
  struct lpc_vector *n, *l;
  
  l = lpc_leaf_new(key, new);
  if (! l)
    return LAGOPUS_RESULT_NO_MEMORY;

  n = get_child(lp, get_index(key, lp));

  if (n) {
    struct lpc_vector *tn;
    
    tn = lpc_node_new(key, (u_char)__fls(key ^ n->key), 1);
    if (! tn) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    
    node_init_parent(tn, lp);
    lpc_put_child(tn, get_index(key, tn) ^ 1, n);
    lpc_put_child_root(lp, key, tn);
    lpc_node_set_parent(n, tn);
    lp = tn;
  }

  node_init_parent(l, lp);
  lpc_put_child_root(lp, key, l);
  lpc_rebalance(t, lp);

  return 0;
}

static int
lpc_insert_info(struct lpc *t, struct lpc_vector *lp, struct lpc_vector *l,
                struct lpc_info *new, struct lpc_info *li, uint32_t key)
{
  if (!l)
    return lpc_insert_node(t, lp, new, key);

  if (li) {
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  } else {
    struct lpc_info *last;

    qlist_for_each_entry(last, &l->leaf, qlist) {
      if (new->slen < last->slen)
        break;
      li = last;
    }

    if (li) {
      qlist_add_behind(&new->qlist, &li->qlist);
    } else {
      qlist_add_head(&new->qlist, &l->leaf);
    }
  }
  
  if (l->slen < new->slen) {
    l->slen = new->slen;
    lpc_leaf_push_suffix(lp, l);
  }

  return 0;
}

static void
lpc_remove_info(struct lpc *t, struct lpc_vector *lp,
                struct lpc_vector *l, struct lpc_info *old)
{
  struct qlist_node **pprev = old->qlist.pprev;
  struct lpc_info *li = qlist_entry(pprev, typeof(*li), qlist.next);

  qlist_del(&old->qlist);
  free(old);
  lpc_info_alloc--;

  if (qlist_empty(&l->leaf)) {
    lpc_put_child_root(lp, l->key, NULL);
    lpc_node_free(l);
    lpc_rebalance(t, lp);
    return;
  }

  if (*pprev)
    return;

  l->slen = li->slen;
  lpc_leaf_pull_suffix(lp, l);
}

static struct lpc_vector *
lpc_find_node(struct lpc *t, struct lpc_vector **lp, uint32_t key)
{
  struct lpc_vector *pn;
  struct lpc_vector *n = t->lv;
  unsigned long index = 0;

  do {
    pn = n;
    n = get_child(n, index);

    if (! n)
      break;

    index = get_cindex(key, n);

    if (index >= (1ul << n->bits)) {
      n = NULL;
      break;
    }
  } while (IS_NODE(n));

  *lp = pn;

  return n;
}

static struct lpc_info *
lpc_find_info(struct qlist_head *lih, u_char slen)
{
  struct lpc_info *li;

  if (!lih) {
    printf("no lih\n");
    return NULL;
  }

  qlist_for_each_entry(li, lih, qlist) {
    if (li->slen < slen)
      continue;
    if (li->slen != slen)
      break;
    return li;
  }

  return NULL;
}

/* API. */
int
lpc_table_insert(struct lpc *t, struct in_addr addr, u_char plen)
{
  struct lpc_info *li = NULL;
  struct lpc_info *new_li;
  struct lpc_vector *l;
  struct lpc_vector *lp;
  uint32_t key = htonl(addr.s_addr);
  u_char slen = (u_char)(KEYLENGTH - plen);
  
  if (plen > KEYLENGTH)
    return LAGOPUS_RESULT_INVALID_ARGS;
  
  if ((plen < KEYLENGTH) && (key << plen))
    return LAGOPUS_RESULT_INVALID_ARGS;

  l = lpc_find_node(t, &lp, key);
  li = l ? lpc_find_info(&l->leaf, slen) : NULL;
  
  if (li) {
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  }

  new_li = (struct lpc_info *)calloc(sizeof(struct lpc_info), 1);
  if (! new_li) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  new_li->slen = slen;
  lpc_info_alloc++;
  lpc_insert_info(t, lp, l, new_li, li, key);
  
  return 0;
}

int
lpc_table_delete(struct lpc *t, struct in_addr addr, u_char plen)
{
  struct lpc_info *li, *li_to_delete;
  struct lpc_vector *l, *lp;
  u_char slen = (u_char)(KEYLENGTH - plen);
  uint32_t key = htonl(addr.s_addr);
  
  if (plen > KEYLENGTH)
    return LAGOPUS_RESULT_INVALID_ARGS;
  
  if ((plen < KEYLENGTH) && (key << plen))
    return LAGOPUS_RESULT_INVALID_ARGS;
  
  l = lpc_find_node(t, &lp, key);
  if (!l) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  
  li = lpc_find_info(&l->leaf, slen);
  if (!li) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  
  li_to_delete = NULL;
  qlist_for_each_entry_from(li, qlist) {
    if (li->slen == slen) {
      li_to_delete = li;
      break;
    }
  }
  
  if (!li_to_delete) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  
  lpc_remove_info(t, lp, l, li_to_delete);

  return 0;
}

int
lpc_table_lookup(struct lpc *t, struct in_addr *daddr,
                 struct lpc_vector **lv, u_char *plen)
{
  const uint32_t key = ntohl(daddr->s_addr);
  struct lpc_vector *n, *pn;
  struct lpc_info *li;
  unsigned long index;
  uint32_t cindex;

  pn = t->lv;
  cindex = 0;

  n = get_child(pn, cindex);
  if (! n)
    return LAGOPUS_RESULT_NOT_FOUND;

  for (;;) {
    index = get_cindex(key, n);

    if (index >= (1ul << n->bits))
      break;

    if (IS_LEAF(n))
      goto found;

    if (n->slen > n->pos) {
      pn = n;
      cindex = (uint32_t)index;
    }

    n = get_child(n, index);
    if (! n)
      goto backtrace;
  }

  for (;;) {
    struct lpc_vector **cptr = n->node;

    if (lpc_prefix_mismatch(key, n) || (n->slen == n->pos))
      goto backtrace;

    if (IS_LEAF(n))
      break;

    while ((n = *cptr) == NULL) {
   backtrace:
      while (!cindex) {
        uint32_t pkey = pn->key;

        if (IS_TOP(pn)) {
          return LAGOPUS_RESULT_NOT_FOUND;
        }
        pn = node_parent(pn);
        cindex = (uint32_t)get_index(pkey, pn);
      }

      cindex &= cindex - 1;
      cptr = &pn->node[cindex];
    }
  }

found:
  index = key ^ n->key;

  qlist_for_each_entry(li, &n->leaf, qlist) {
    if ((index >= (1ul << li->slen)) && (li->slen != KEYLENGTH))
      continue;

    *lv = n;
    *plen = (u_char)(KEYLENGTH - li->slen);
    return 0;
  }
  goto backtrace;
}

/* Initialize LPC top. */
struct lpc *
lpc_init(void)
{
  struct lpc *lpc;

  lpc = calloc(1, sizeof(struct lpc));
  if (lpc == NULL) {
    return NULL;
  }

  lpc->lv[0].pos = KEYLENGTH;
  lpc->lv[0].slen = KEYLENGTH;

  return lpc;
}

void
lpc_free(struct lpc *lpc)
{
  free(lpc);
}

/* Dump API. */
void
lpc_table_dump(struct lpc_vector *lv)
{
  int verbose = 0;
  struct in_addr addr;
  
  addr.s_addr = htonl(lv->key);

  if (IS_TOP(lv)) {
    if (verbose)
      printf("TOP(%p) %s pos=%d bits=%d slen=%d\n",
             lv, inet_ntoa(addr), lv->pos, lv->bits, lv->slen);
    if (lv->node[0]) {
      lpc_table_dump(lv->node[0]);
    }
  } else if (IS_NODE(lv)) {
    int i;
    if (verbose)
      printf("NODE(%p) %s pos=%d bits=%d slen=%d\n",
             lv, inet_ntoa(addr), lv->pos, lv->bits, lv->slen);

    for (i = 0; i < (1 << lv->bits); i++) {
      if (lv->node[i]) {
        lpc_table_dump(lv->node[i]);
      } else {
        if (verbose) {
          printf("Empty node index %d\n", i);
        }
      }
    }
  } else if (IS_LEAF(lv)) {
    struct lpc_info *fi;
    struct lpc_info *prev;
    if (verbose)
      printf("LEAF(%p) %s pos=%d bits=%d slen=%d\n",
             lv, inet_ntoa(addr), lv->pos, lv->bits, lv->slen);
    qlist_for_each_entry(fi, &lv->leaf, qlist)
        if (! fi->qlist.next)
          break;
    qlist_reverse_each_entry(fi, prev, &lv->leaf, qlist) {
      printf("%s/%d\n", inet_ntoa(addr), 32 - fi->slen);
    }
  }
}
