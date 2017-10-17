#ifndef  UTIL_H
#define  UTIL_H
#include "ElementaryClasses.h"
#include "int_vector.h"
#include "rule_vector.h"
#include "u_multiset.h"

#define WI_LOW 0
#define WI_HIGH 1

typedef struct ival{
	unsigned int first;
	unsigned int second;
} ival;

typedef struct weighted_interval{
	ival *ival;
	int weight;
	rule **rules;
	int num_rules;
	int  field;
} weighted_interval;

typedef struct light_weighted_interval {
	unsigned int a;
	unsigned int b;
	int w;
	int_vector *rule_indices;
} light_weighted_interval;


bool iu_interval_is_identical(const rule *r1, const rule *r2);
int iu_get_max_overlap(u_multiset *lo, u_multiset *hi);
void mwis_intervals(weighted_interval **I, const int NUM_INTERVALS, int_vector **max_set, int *max);
void fast_mwis_intervals(light_weighted_interval **I, const int NUM_INTERVALS, int_vector **max_set, int *max);
void mwis_intervals_rules(rule_vector *I, int x, int_vector **max_set, int *max);
void create_unique_interval(rule_vector *rules, int field, weighted_interval ***intervals, int *num_intervals);
void fast_create_unique_interval(interval *rules, const int NUM_RULES, light_weighted_interval ***out, int *num_intervals);
rule_vector * iu_redundancy_removal(rule_vector *rules);


static inline unsigned int lwi_get_low(light_weighted_interval *lwi) { return lwi->a; }
static inline unsigned int lwi_get_high(light_weighted_interval *lwi) { return lwi->b; }
static inline unsigned int lwi_get_weight(light_weighted_interval *lwi) { return lwi->w; }
static inline light_weighted_interval * lwi_create(void){
	light_weighted_interval *lwi = (light_weighted_interval *)safe_malloc(sizeof(light_weighted_interval));
	lwi->rule_indices = int_vector_init();
	return lwi;
}
static inline void lwi_destroy(light_weighted_interval *lwi){
	int_vector_free(lwi->rule_indices);
	free(lwi);
}
static inline void lwi_push(light_weighted_interval *lwi, int x) {
	int_vector_push_back(lwi->rule_indices, x);
}
static inline void lwi_get_rule_indices(light_weighted_interval *lwi, int **rule_indices, int *num_indices) {
	*rule_indices = int_vector_to_array(lwi->rule_indices);
	*num_indices = int_vector_size(lwi->rule_indices);
}



static inline int iu_count_pom(weighted_interval *wi, int second_field) {
	u_multiset *a_start = u_multiset_create();
	u_multiset *a_end = u_multiset_create();

	for (int i = 0; i < wi->num_rules; ++i) {
		u_multiset_insert(a_start, wi->rules[i]->range[second_field][WI_LOW], 1);
		u_multiset_insert(a_end, wi->rules[i]->range[second_field][WI_HIGH], 1);
	}
	int max_overlap = iu_get_max_overlap(a_start, a_end);

	u_multiset_destroy(a_start);
	u_multiset_destroy(a_end);

	return max_overlap;
}
static inline void iu_set_weight_by_penalty_pom(weighted_interval *wi, int second_field) {
	int penalty_pom = wi->num_rules - 100 * iu_count_pom(wi, second_field);
	wi->weight = penalty_pom > 1 ? penalty_pom : 1;
}
static inline void iu_set_weight_by_size_plus_one(weighted_interval *wi) {
	wi->weight = wi->num_rules +1;
}
static inline void iu_set_weight_by_size(weighted_interval *wi) {
	wi->weight = wi->num_rules;
}
static inline weighted_interval * weighted_interval_create_i(rule **rules, const int NUM_RULES, unsigned int a, unsigned int b) {
	if (NUM_RULES == 0) {
		printf("ERROR: EMPTY RULE AND CONSTRUCT INTERVAL?\n");
		exit(0);
	}
	weighted_interval *wi = (weighted_interval*)safe_malloc(sizeof(weighted_interval));	
	wi->weight = 100000;	
	wi->rules = rules;
	wi->num_rules = NUM_RULES;
	wi->ival = (ival *)safe_malloc(sizeof(ival));
	wi->ival->first = a;
	wi->ival->second = b;
	iu_set_weight_by_size_plus_one(wi);
	wi->field = 0;
	return wi;
}
static inline weighted_interval * weighted_interval_create_f(rule **rules, const int NUM_RULES, int field) {
	if (NUM_RULES == 0) {
		printf("ERROR: EMPTY RULE AND CONSTRUCT INTERVAL?\n");
		exit(0);
	}
	weighted_interval *wi = (weighted_interval*)safe_malloc(sizeof(weighted_interval));		
	wi->weight = 100000;	
	wi->rules = rules;
	wi->num_rules = NUM_RULES;
	wi->field = field;
	wi->ival = (ival *)safe_malloc(sizeof(ival));
	wi->ival->first = rules[0]->range[field][WI_LOW];
	wi->ival->second = rules[0]->range[field][WI_HIGH];
	iu_set_weight_by_size_plus_one(wi);
	return wi;
}
static inline void weighted_interval_destroy(weighted_interval *wi){
	free(wi->rules);
	free(wi->ival);
	free(wi);
}
static inline unsigned int wi_get_low(weighted_interval *wi) { return wi->ival->first; }
static inline unsigned int wi_get_high(weighted_interval *wi) { return wi->ival->second; }
static inline rule ** wi_get_rules(weighted_interval *wi) { return wi->rules; }
static inline int wi_get_num_rules(weighted_interval *wi) { return wi->num_rules; }
static inline int wi_get_field(weighted_interval *wi) { return wi->field; }
static inline int wi_get_weight(weighted_interval *wi) { return wi->weight; }

#endif
