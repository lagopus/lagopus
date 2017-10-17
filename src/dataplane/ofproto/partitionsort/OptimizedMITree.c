#include "OptimizedMITree.h"

optimized_mi_tree * optimized_mi_tree_init_sr( sortable_ruleset *rules) {
	optimized_mi_tree *tree = (optimized_mi_tree *)safe_malloc(sizeof(optimized_mi_tree));
	tree->counter = 0;
	tree->is_mature = false;
	tree->num_rules = 0;
	tree->root = rb_tree_create();
	tree->priority_container = u_multiset_create();
	tree->field_order = int_vector_initv(rules->field_order);
	tree->max_priority = -1;
	rule_vector *rules_v = srp_get_rule(rules);
	rule **rules_a = rule_vector_to_array(rules_v);
	for (int i = 0; i < rule_vector_size(rules_v); ++i) {
		bool priority_change = false;
		mit_insertion_rp(tree, rules_a[i], &priority_change);
	}
	return tree;
}

optimized_mi_tree * optimized_mi_tree_init_fo(int_vector *field_order){
	optimized_mi_tree *tree = (optimized_mi_tree *)safe_malloc(sizeof(optimized_mi_tree));
	tree->counter = 0;
	tree->is_mature = false;
	tree->field_order = field_order;
	tree->root = rb_tree_create();
	tree->num_rules = 0; 
	tree->max_priority = -1;
	tree->priority_container = u_multiset_create();
	return tree;
}

optimized_mi_tree * optimized_mi_tree_init_r( rule *r){
	optimized_mi_tree *tree = (optimized_mi_tree *)safe_malloc(sizeof(optimized_mi_tree));
	tree->counter = 0;
	tree->is_mature = false;
	tree->root = rb_tree_create();
	tree->num_rules = 0; 
	int *field_order_a = NULL;
	int num_fields = 0;
	get_field_order_by_rule(r, &field_order_a, &num_fields);
	tree->field_order = int_vector_inita(field_order_a, num_fields);
	free(field_order_a);	
	tree->max_priority = -1;
	tree->priority_container = u_multiset_create();
	return tree;
}

optimized_mi_tree * optimized_mi_tree_init_d(){
	optimized_mi_tree *tree = (optimized_mi_tree *)safe_malloc(sizeof(optimized_mi_tree));
	tree->counter = 0;
	tree->is_mature = false;
	tree->num_rules = 0;
	tree->root = rb_tree_create();
	int num_fields = 4;
	int *field_order_a = (int *)safe_malloc(sizeof(int) * num_fields);
	tree->field_order = int_vector_inita(field_order_a, num_fields);
	tree->max_priority = -1;
	tree->priority_container = u_multiset_create();
	return tree;
}

void optimized_mi_tree_destroy(optimized_mi_tree *tree) {
	rb_tree_destroy(tree->root);
	int_vector_free(tree->field_order);
	u_multiset_destroy(tree->priority_container);
	free(tree);
}

void mit_insertion_r(optimized_mi_tree *tree,  rule *r) {
	u_multiset_insert(tree->priority_container, r->master->priority, 1); 
	tree->max_priority = tree->max_priority > r->master->priority ? tree->max_priority : r->master->priority;
	rb_tree_insert_with_path_compression(tree->root, r->range, r->dim, 0, int_vector_to_array(tree->field_order), int_vector_size(tree->field_order), r);
	tree->num_rules++;
	tree->counter++;
}

void mit_insertion_rp(optimized_mi_tree *tree,  rule *r, bool *priority_change) {
//	if (mit_can_insert_rule(rule)) {
		u_multiset_insert(tree->priority_container, r->master->priority, 1); 
		*priority_change = r->master->priority > tree->max_priority;
		tree->max_priority = *priority_change ? tree->max_priority : r->master->priority;
		rb_tree_insert_with_path_compression(tree->root, r->range, r->dim, 0, int_vector_to_array(tree->field_order), int_vector_size(tree->field_order), r);
		tree->num_rules++;
		tree->counter++;
//		} 
}

bool mit_try_insertion(optimized_mi_tree *tree,  rule *r, bool *priority_change) {
	if (mit_can_insert_rule(tree, r)) {
		u_multiset_insert(tree->priority_container, r->master->priority, 1); 
		*priority_change = r->master->priority > tree->max_priority;
		tree->max_priority = *priority_change ? tree->max_priority : r->master->priority;
		rb_tree_insert_with_path_compression(tree->root, r->range, r->dim, 0, int_vector_to_array(tree->field_order), int_vector_size(tree->field_order), r);
		tree->num_rules++;
		tree->counter++;
		return true;
	} else { return false; }
}

void mit_deletion(optimized_mi_tree *tree,  rule *r, bool *priority_change) {
	//printf("**>\n");
	//auto pit = priority_container.equal_range(rule.priority);
	//priority_container.erase(pit.first);

	u_multiset_delete_equal(tree->priority_container, r->master->priority);

	if (tree->num_rules == 1) {
		tree->max_priority = -1;
		*priority_change = true;
	} else if (r->master->priority == tree->max_priority) {
		*priority_change = true;
		tree->max_priority = u_multiset_max(tree->priority_container);
	}
	tree->num_rules--;
	bool just_deleted_tree = false;
	rb_tree_delete_with_path_compression(&tree->root, r->range, 0, int_vector_to_array(tree->field_order), int_vector_size(tree->field_order), r->priority, &just_deleted_tree);
	//printf("<**\n");
}

bool mit_is_identical_vector( int *lhs,  int *rhs,  int NUM_FIELDS);
bool mit_is_identical_vector( int *lhs,  int *rhs,  int NUM_FIELDS) {
	for (size_t i = 0; i < NUM_FIELDS; ++i) {
		if (lhs[i] != rhs[i]) return false;
	}
	return true;
}

void reconstruct_if_num_rules_leq(optimized_mi_tree *tree, int threshold) {
	if (tree->is_mature) return;
	if (tree->num_rules >= threshold) {
		tree->is_mature = true;  return;
	}
	//global_counter++;

	rule_vector *serialized_rules = mit_serialize_into_rules(tree); 
	bool size_change = false;
	int_vector *newFO = NULL;
	fast_greedy_field_selection_for_adaptive(serialized_rules, &size_change, &newFO);
	rule **serialized_rulesA = rule_vector_to_array(serialized_rules);
	if (!size_change || mit_is_identical_vector(int_vector_to_array(tree->field_order), int_vector_to_array(newFO), int_vector_size(tree->field_order))){
		for (int i = 0; i < rule_vector_size(serialized_rules); ++i) {//Must make sure to delete serialized rules to prevent memory leak
			rule_destroy(serialized_rulesA[i]);
		}
		rule_vector_free(serialized_rules);
		int_vector_free(newFO);
		return;
	}

	mit_reset(tree);

	int_vector_free(tree->field_order);
	tree->field_order = newFO;

	for (int i = 0; i < rule_vector_size(serialized_rules); ++i) {
		mit_insertion_r(tree, serialized_rulesA[i]);
		rule_destroy(serialized_rulesA[i]);//Must make sure to delete serialized rules to prevent memory leak
	}

	rule_vector_free(serialized_rules);
}


void mit_reset(optimized_mi_tree *tree) {

	rb_tree_destroy(tree->root);
	tree->num_rules = 0; 
	int_vector_clear(tree->field_order);
	u_multiset_clear(tree->priority_container);
	tree->max_priority = -1;
	tree->root = rb_tree_create();

}
