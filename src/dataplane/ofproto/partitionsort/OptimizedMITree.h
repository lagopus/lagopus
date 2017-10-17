#ifndef  OPTMITREE_H
#define  OPTMITREE_H

#include "red_black_tree.h"
#include "SortableRulesetPartitioner.h"
#include "box_vector.h"
#include "ll_vector.h"
#include "u_multiset.h"

typedef struct optimized_mi_tree {

	bool is_mature;
	rb_red_blk_tree * root;
	int counter;
	int num_rules;
	int_vector *field_order;
	u_multiset *priority_container;
	long long max_priority;
	
} optimized_mi_tree;

optimized_mi_tree * optimized_mi_tree_init_sr( sortable_ruleset *rules);
optimized_mi_tree * optimized_mi_tree_init_fo(int_vector *field_order);
optimized_mi_tree * optimized_mi_tree_init_r( rule *r);
optimized_mi_tree * optimized_mi_tree_init_d(void);
void optimized_mi_tree_destroy(optimized_mi_tree *);
void mit_insertion_r(optimized_mi_tree *tree,  rule *rule);
void mit_insertion_rp(optimized_mi_tree *tree,  rule *rule, bool *priority_change);
bool mit_try_insertion(optimized_mi_tree *tree,  rule *rule, bool *priority_change);
void mit_deletion(optimized_mi_tree *tree,  rule *rule, bool *priority_change);

static inline bool mit_can_insert_rule(optimized_mi_tree *tree,  rule *r){	
	return rb_tree_can_insert(tree->root, r->range, r->dim, 0, int_vector_to_array(tree->field_order), int_vector_size(tree->field_order));
}

static inline void mit_get_field_order(optimized_mi_tree *tree, int **fO, int *numFields){
	*fO = int_vector_to_array(tree->field_order);
	*numFields = int_vector_size(tree->field_order);
}

static inline void mit_print(optimized_mi_tree *tree) { rb_tree_print(tree->root); };
static inline void mit_print_field_order(optimized_mi_tree *tree) {
	int *fOA = NULL;
	int num_fields = 0;
	mit_get_field_order(tree, &fOA, &num_fields);
	for (size_t i = 0; i < num_fields; ++i) {
		printf("%d ", fOA[i]);
	}
	printf("\n");
}
static inline rule * mit_classify_a_packet(optimized_mi_tree *tree,  packet one_packet) {
	return  rb_exact_query_iterative(tree->root, one_packet,  int_vector_to_array(tree->field_order), int_vector_size(tree->field_order));
}
static inline rule * mit_classify_a_packet_p(optimized_mi_tree *tree,  packet one_packet, rule * best_rule_so_far) {
	return  rb_exact_query_priority(tree->root, one_packet, 0, int_vector_to_array(tree->field_order), int_vector_size(tree->field_order), best_rule_so_far);
}

static inline size_t mit_num_rules(optimized_mi_tree *tree) { return tree->num_rules; }
static inline int mit_max_priority(optimized_mi_tree *tree) { return tree->max_priority; }
static inline bool mit_empty(optimized_mi_tree *tree) { return u_multiset_empty(tree->priority_container); }

void reconstruct_if_num_rules_leq(optimized_mi_tree *tree, int threshold);

static inline rule_vector* mit_serialize_into_rules(optimized_mi_tree *tree) {
	int *fOA = NULL;
	int num_fields = 0;
	mit_get_field_order(tree, &fOA, &num_fields);
	return rb_serialize_into_rules(tree->root, fOA, num_fields);
}

static inline rule_vector* mit_get_rules(optimized_mi_tree *tree) {
	return mit_serialize_into_rules(tree);
}

static inline int mit_memory_consumption(optimized_mi_tree *tree) {
	int *fOA = NULL;
	int num_fields = 0;
	mit_get_field_order(tree, &fOA, &num_fields);
	return rb_calculate_memory_consumption(tree->root, fOA, num_fields);
}

static inline memory MITMemSizeBytes(optimized_mi_tree *tree) {
	return mit_memory_consumption(tree);
}


void mit_reset(optimized_mi_tree *tree);



#endif
