#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifndef  RBTREE_H
#define  RBTREE_H

#include "../utilities/misc.h"
#include "../utilities/stack.h"
#include "../utilities/ElementaryClasses.h"
#include "../utilities/box_vector.h"
#include "../utilities/rule_vector.h"

/*  CONVENTIONS:  All data structures for red-black trees have the prefix */
/*                "rb_" to prevent name conflicts. */
/*                                                                      */
/*                Function names: Each word in a function name begins with */
/*                a capital letter.  An example funcntion name is  */
/*                CreateRedTree(a,b,c). Furthermore, each function name */
/*                should begin with a capital letter to easily distinguish */
/*                them from variables. */
/*                                                                     */
/*                Variable names: Each word in a variable name begins with */
/*                a capital letter EXCEPT the first letter of the variable */
/*                name.  For example, int newLongInt.  Global variables have */
/*                names beginning with "g".  An example of a global */
/*                variable name is gNewtonsConstant. */

/* comment out the line below to remove all the debugging assertion */
/* checks from the compiled code.  */
#define DEBUG_ASSERT 0
#define BOX_SIZE 2

static const int LOW = 0, HIGH = 1;
struct rb_red_blk_tree;
//typedef std::array<Point, 2>  box; 
typedef union rule_specifier box; //Box will always point to an array of BOX_SIZE points

//Total 25 bytes per node 
typedef struct rb_red_blk_node {
	box key;
	struct rb_red_blk_node* left;
	struct rb_red_blk_node* right;
	struct rb_red_blk_tree * rb_tree_next_level;
	//int priority; /*max_priority of all children*/
	struct rb_red_blk_node* parent;
	bool red; /* if red=0 then the node is black */

} rb_red_blk_node; 


//Total 
typedef struct rb_red_blk_tree { 
  /*  A sentinel is used for root and for nil.  These sentinels are */
  /*  created when RBTreeCreate is caled.  root->left should always */
  /*  point to the node which is the root of the tree.  nil points to a */
  /*  node which should always be black but has aribtrary children and */
  /*  parent and no key or info.  The point of using these sentinels is so */
  /*  that the root and nil nodes do not require special cases in the code */
  rb_red_blk_node* root;             
  rb_red_blk_node* nil;   
   
  int count;

//  box *chain_boxes;
//  int num_boxes;
  struct box_vector *chain_boxes;


  //std::priority_queue<int> pq;

//  int *rule_list;
//  int num_priorities;
//  int max_num_priorities;
  struct rule_vector *rule_list;

  rule *max_priority_rule;

} rb_red_blk_tree;

/**
FOR RB tree light weight node
**/
rb_red_blk_tree* rb_tree_create(void);
rb_red_blk_node * rb_tree_insert_with_path_compression(rb_red_blk_tree* tree, const box *key, const int NUM_BOXES, unsigned int level, const int *field_order, const int NUM_FIELDS, rule *r);
void rb_tree_delete_with_path_compression(rb_red_blk_tree** tree, const box *key, int level, const int *field_order, const int NUM_FIELDS, long long priority, bool *just_deleted_tree);
bool rb_tree_insert_with_path_compression_help(rb_red_blk_tree* tree, rb_red_blk_node* z, const box *b, const int NUM_BOXES, int level, const int *field_order, const int NUM_FIELDS, rule *r, rb_red_blk_node** out_ptr);
rule * rb_exact_query_priority(rb_red_blk_tree*  tree, const packet q, int level, const int *field_order, const int NUM_FIELDS, rule * best_rule_so_far); 
bool rb_tree_insert_help(rb_red_blk_tree* tree, rb_red_blk_node* z, const box *b, const int B_SIZE, int level, const int *field_order, rule *r, rb_red_blk_node* out_ptr);
rb_red_blk_node * rb_tree_insert(rb_red_blk_tree* tree, const box *key, const int KEY_SIZE, int level, const int *field_order, rule *r);
bool rb_tree_can_insert(rb_red_blk_tree *tree, const box *z, const int NUM_BOXES, int level, const int *field_order, const int NUM_FIELDS);
void rb_delete(rb_red_blk_tree* , rb_red_blk_node* );
void rb_tree_destroy(rb_red_blk_tree*);
rb_red_blk_node* rb_tree_predecessor(rb_red_blk_tree*,rb_red_blk_node*);
rb_red_blk_node* rb_tree_successor(rb_red_blk_tree*,rb_red_blk_node*);
void  rb_serialize_into_rules_recursion(rb_red_blk_tree * treenode, rb_red_blk_node * node, int level, const int *field_order, const int NUM_FIELDS, box_vector *box_so_far, rule_vector *rules_so_far);
rule_vector* rb_serialize_into_rules(rb_red_blk_tree* tree, const int *field_order, const int NUM_FIELDS);
rule * rb_exact_query_iterative(rb_red_blk_tree*  tree, const packet q, const int *field_order, const int NUM_FIELDS);
rb_red_blk_node* rb_node_create(void);
void rb_node_destroy(rb_red_blk_node *);
void rb_pop_rule(struct rb_red_blk_tree *rb_t, long long p);

static inline void rb_push_rule(struct rb_red_blk_tree *rb_t, rule *r) {

  if(rb_t->max_priority_rule != NULL)
  	rb_t->max_priority_rule = r->master->priority > rb_t->max_priority_rule->master->priority ? r : rb_t->max_priority_rule;
  else
	rb_t->max_priority_rule = r;

  rule_vector_push_back(rb_t->rule_list, r);
}

static inline void rb_clear_rule(struct rb_red_blk_tree *rb_t) {
  rb_t->max_priority_rule = NULL;
  rule_vector_clear(rb_t->rule_list);
}

//static inline void rb_print_key(const box b) { printf("[%u %u]\n", b.range[0], b.range[1]); }
static inline int rb_get_size_list(struct rb_red_blk_tree *rb_t) { return rule_vector_size(rb_t->rule_list); }
static inline bool rb_overlap(unsigned int a1, unsigned  int a2, unsigned int b1, unsigned int b2) { return a1 <= b1 ? (b1 <= a2) : (a1 <= b2); }

#endif
