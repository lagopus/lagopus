#include "generic_classifier.h"

struct generic_classifier * generic_classifier_init(){
	struct generic_classifier *c = (struct generic_classifier *)calloc(1, sizeof(struct generic_classifier));
	c->rules = rule_vector_init();
	return c;
}

void generic_classifier_destroy(struct generic_classifier *c){
	rule_vector_free(c->rules);
	free(c);
}

void generic_classifier_construct(struct generic_classifier *c, rule_vector *rules){
	int i;

	if(rule_vector_size(c->rules) != 0){ // Did not delete before constructing
		rule_vector_clear(c->rules);
		memset(c->rule_cache, NULL, sizeof(c->rule_cache));
	}
	rule_vector_push_range(c->rules, rules);
	generic_classifier_insertion_sort_rules(c);
}

void generic_classifier_delete(struct generic_classifier *c){	
	rule_vector_clear(c->rules);
	memset(c->rule_cache, NULL, sizeof(c->rule_cache));
}

void generic_classifier_rule_insert(struct generic_classifier *c, struct rule *r){
	rule_vector_push_back(c->rules, r);
	generic_classifier_insertion_sort_rules(c);
}

void generic_classifier_rule_delete(struct generic_classifier *c, struct rule *r){
	rule_vector_erase(c->rules, rule_vector_find(c->rules, r));
}

struct flow * generic_classifier_classify_packet(struct generic_classifier *c, packet p, int current_best){
	int i;
	struct rule *match;

	for(i = rule_vector_size(c->rules) - 1; i >= 0; --i){
		match = rule_vector_get(c->rules, i);
		if(match->priority < current_best)
			break;
		if(discontiguous_rule_matches_packet(match, p)){
			return match->master;
		}
	}

	return NULL;
}

void generic_classifier_insertion_sort_rules(struct generic_classifier *c){
	if(rule_vector_size(c->rules) > 1){
		int sorting_index,unsorted_index, num_rules;
		for(unsorted_index = 1, num_rules = rule_vector_size(c->rules); unsorted_index < num_rules; ++unsorted_index){
			for(sorting_index = unsorted_index - 1; sorting_index >= 0; --sorting_index){
				if(rule_vector_get(c->rules, sorting_index + 1)->priority < rule_vector_get(c->rules, sorting_index)->priority){
					rule_vector_iswap(c->rules, sorting_index, sorting_index + 1);
				}
			}
		}
	}
}

