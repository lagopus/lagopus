#ifndef U_MULTISET
#define U_MULTISET

#ifdef DMALLOC
#include <dmalloc.h>
#endif
#include"misc.h"
#include"stack.h"
#include <stdint.h>

//Unsigned multiset using Emin Martinian's red-black tree implementation

typedef struct u_multiset_node {
  unsigned key;
  unsigned multiplicity;
  int red; /* if red=0 then the node is black */
  struct u_multiset_node* left;
  struct u_multiset_node* right;
  struct u_multiset_node* parent;
} u_multiset_node;

typedef struct u_multiset{
  /*  A sentinel is used for root and for nil.  These sentinels are */
  /*  created when UMultisetCreate is caled.  root->left should always */
  /*  point to the node which is the root of the tree.  nil points to a */
  /*  node which should always be black but has aribtrary children and */
  /*  parent and no key or info.  The point of using these sentinels is so */
  /*  that the root and nil nodes do not require special cases in the code */
  u_multiset_node* root;             
  u_multiset_node* nil;
  int size;
} u_multiset;

u_multiset* u_multiset_create(void);
u_multiset_node * u_multiset_insert(u_multiset*,  unsigned key, unsigned multiplicity);
void u_multiset_delete(u_multiset* ,  unsigned key);
void u_multiset_delete_equal(u_multiset* tree,  unsigned key);
void u_multiset_destroy(u_multiset*);
u_multiset_node* u_multiset_tree_predecessor(u_multiset*,u_multiset_node*);
u_multiset_node* u_multiset_tree_successor(u_multiset*,u_multiset_node*);
u_multiset_node* u_multiset_exact_query(u_multiset*, unsigned);
stk_stack * u_multiset_enumerate(u_multiset* tree,  unsigned low,  unsigned high);
void u_multiset_clear(u_multiset*);
int u_multiset_empty(u_multiset*);
int u_multiset_size(u_multiset*);
unsigned u_multiset_max(u_multiset* tree);
unsigned * u_multiset_create_array(u_multiset*);
void null_function(void*);

#endif
