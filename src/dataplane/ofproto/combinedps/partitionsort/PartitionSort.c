#include "PartitionSort.h"
#include "ll_vector.h"


struct partition_sort * ps_init(){
	struct partition_sort *ps = (struct partition_sort *)malloc(sizeof(struct partition_sort));
	ps->_tree_cap = 5;
	ps->_rules_cap = 1000;
	ps->mitrees = (optimized_mi_tree **)calloc(ps->_tree_cap, sizeof(optimized_mi_tree *));//Ensure memory is zeroed out
	ps->rules = (rt_pair *)calloc(ps->_rules_cap, sizeof(rt_pair));//Ensure memory is zeroed out
	ps->num_trees = 0;
	ps->num_rules = 0;
	return ps;
}

void ps_destroy(struct partition_sort *ps){
	ps_classifier_delete(ps);
	free(ps->mitrees);
	free(ps->rules);
	free(ps);
}

void ps_classifier_delete(struct partition_sort *ps) {
	for (unsigned i = 0; i < ps->num_trees; ++i) {
		if(ps->mitrees[i] != NULL){
			optimized_mi_tree_destroy(ps->mitrees[i]);
			ps->mitrees[i] = NULL;
		}
	}
	ps->num_trees = 0;
	ps->num_rules = 0;
}

void ps_classifier_construct_online(struct partition_sort *self,  struct rule **rules, const unsigned num_rules)  {

	if(self == NULL || self->rules == NULL) // Nothing to construct, not initialized
		return; 

	for (unsigned i = 0; i < num_rules; ++i) {
		ps_rule_insert(self, rules[i]);//Initial classifier is not sorted, need to sort
	}

}

void ps_classifier_construct_offline(struct partition_sort *self,  struct rule **rules, const unsigned num_rules) { 

	sortable_ruleset **buckets = NULL;
	rule_vector *rules_v = rule_vector_initr(rules, num_rules);
	sortable_ruleset_partitioning_gfs(rules_v, &buckets, &self->num_trees);

	self->_tree_cap = self->num_trees;

	self->mitrees = (optimized_mi_tree **)realloc(self->mitrees, self->num_trees * sizeof( optimized_mi_tree *));
	for (int i = 0; i < self->num_trees; ++i)  {		
		self->mitrees[i] = optimized_mi_tree_init_sr(buckets[i]);
	}
//	ps_insertion_sort_mitrees(self);

	rule_vector_free(rules_v);
	for(int i = 0; i < self->num_trees; ++i){
		sortable_ruleset_destroy(buckets[i]);
	}
	free(buckets);
}

void ps_rule_insert(struct partition_sort *ps,  struct rule *one_rule) {

	for (unsigned i = 0; i < ps->num_trees; ++i)
	{
		bool priority_change = false;
		
		bool success = mit_try_insertion(ps->mitrees[i], one_rule, &priority_change);
		if (success) {
			
//			if (priority_change) {
//				ps_insertion_sort_mitrees(ps);
//			}
			reconstruct_if_num_rules_leq(ps->mitrees[i], 10);
			if(ps->num_rules == ps->_rules_cap){
				ps->rules = (rt_pair *)realloc(ps->rules, sizeof(rt_pair) * ps->_rules_cap * 2);
				ps->_rules_cap *= 2;
			}
			ps->rules[ps->num_rules].first = one_rule;
			ps->rules[ps->num_rules].second = ps->mitrees[i];
			one_rule->id = ps->num_rules; // id keeps track of the index so tree can be found in constant time
			ps->num_rules++;
			return;
		}
	}

	bool priority_change = false;
	 
	optimized_mi_tree *tree_ptr = optimized_mi_tree_init_r(one_rule);
	mit_try_insertion(tree_ptr, one_rule, &priority_change);

	if(ps->num_rules == ps->_rules_cap){
		ps->rules = (rt_pair *)realloc(ps->rules, sizeof(rt_pair) * ps->_rules_cap * 2);
		ps->_rules_cap *= 2;
	}
	ps->rules[ps->num_rules].first = one_rule;
	ps->rules[ps->num_rules].second = tree_ptr;
	one_rule->id = ps->num_rules;
	ps->num_rules++;

	if(ps->num_trees == ps->_tree_cap){
		ps->mitrees = (optimized_mi_tree **)realloc(ps->mitrees, sizeof(optimized_mi_tree *) * ps->_tree_cap * 2);
		ps->_tree_cap *= 2;
	}
	ps->mitrees[ps->num_trees++] = tree_ptr;  
//	ps_insertion_sort_mitrees(ps);
}

void ps_rule_delete(struct partition_sort *ps, struct rule *one_rule){
	int i = one_rule->id; //id stores the index in PartitionSort
	if (i < 0 || i >= ps->num_rules) {
		//printf("Warning index delete rule out of bound: do nothing here\n");
		//printf("%lu vs. size: %u", i, ps->num_rules);
		return;
	}
	bool priority_change = false;

	struct optimized_mi_tree * mitree = ps->rules[i].second; 
	mit_deletion(mitree, ps->rules[i].first, &priority_change); 
 
//	if (priority_change) {
//		ps_insertion_sort_mitrees(ps);
//	}


	if (i != ps->num_rules - 1) {
		ps->rules[i] = ps->rules[ps->num_rules - 1];
		ps->rules[i].first->id = i;
	}
	ps->num_rules--;


}

struct flow * ps_classify_packet(struct partition_sort *ps, const packet p) {

	struct optimized_mi_tree *t = NULL;
	struct flow *match = NULL;
	int result = -1;
	rule * temp_rule = NULL;

	for (unsigned i = 0; i < ps->num_trees; ++i) {

		t = ps->mitrees[i];

//		if (result > t->max_priority){
//			break;
//		}

		temp_rule = mit_classify_a_packet(t, p);

		if(temp_rule != NULL && (result == -1 || temp_rule->master->priority > match->priority)){						
			result = temp_rule->master->priority;
			match = temp_rule->master;
		}	
		
	}

	return match;
	
}

void ps_insertion_sort_mitrees(struct partition_sort *ps){
	int i, j, numLength = ps->num_trees;
	struct optimized_mi_tree * key;
	for (j = 1; j < numLength; ++j)
	{
		key = ps->mitrees[j];
		for (i = j - 1; (i >= 0) && (ps->mitrees[i]->max_priority > key->max_priority); --i)
		{
			ps->mitrees[i + 1] = ps->mitrees[i];
		}
		ps->mitrees[i + 1] = key;
	}
}
