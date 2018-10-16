#ifndef  SORTABLE_H
#define  SORTABLE_H
#include "../utilities/ElementaryClasses.h"
#include "../utilities/IntervalUtilities.h"
#include "../utilities/rule_vector.h"
#include "../utilities/int_vector.h"

typedef struct sortable_ruleset{
	rule_vector *rule_list;
	int_vector *field_order;
} sortable_ruleset;

typedef rule_vector part;

const static int SRLOW = 0 , SRHIGH = 1;
bool is_bucket_really_sortable( sortable_ruleset *b);
int * sort_indexes( int *v, const int SIZE);
sortable_ruleset * sortable_ruleset_assign(sortable_ruleset *a,  sortable_ruleset *b);

/*For GrInd */
//This function takes O(d^2 nlog n) time.
void sortable_ruleset_partitioning_gfs( rule_vector *rules, sortable_ruleset ***partitions, int *num_partitions);
void greedy_field_selection( rule_vector *in_rules, rule_vector **out_rules, int_vector **current_field);
void fast_greedy_field_selection_two_iterations( rule_vector *rules, bool *size, int_vector **current_field);
void fast_greedy_field_selection( rule_vector *in_rules, rule_vector **out_rules, int_vector **current_field);
void fast_greedy_field_selection_for_adaptive( rule_vector *rules, bool *size, int_vector **current_field);
void get_field_order_by_rule( rule *r, int **field_order, int *num_fields);

/*Helper For GrInd */ 
void is_this_partition_sortable( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, bool *is_sortable);
void is_entire_partition_sortable( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, bool *is_sortable);
void mwis_on_partition( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, int *max_MIS);
void mwis_on_entire_partition( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, int *sum_weight);
void best_field_and_configuration(rule_vector ***all_partitions, int *num_partitions, int_vector *current_field, int num_fields, bool do_not_delete);

void fast_mwis_on_partition( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, int *max_MIS);
void fast_mwis_on_entire_partition( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, int *sum_weight);
void fast_best_field_and_configuration(rule_vector ***all_partitions, int *num_partitions, int_vector *current_field, int num_fields, bool do_not_delete);

static inline sortable_ruleset * sortable_ruleset_init(rule_vector *rule_list, int_vector *field_order){
	sortable_ruleset *set = (sortable_ruleset *)safe_malloc(sizeof(sortable_ruleset));
	set->rule_list = rule_list;
	set->field_order = field_order;
}

static inline sortable_ruleset * sortable_ruleset_create( int NUM_FIELDS){
	sortable_ruleset *set = (sortable_ruleset *)safe_malloc(sizeof(sortable_ruleset));
	set->rule_list = rule_vector_init();
	set->field_order = int_vector_initc(NUM_FIELDS);
}

static inline void sortable_ruleset_destroy(sortable_ruleset *set)
{
	rule_vector_free(set->rule_list);
	int_vector_free(set->field_order);
	free(set);
}

static inline  rule_vector* srp_get_rule(sortable_ruleset *set){ return set->rule_list; }
static inline  int* srp_get_field_ordering(sortable_ruleset *set){ return int_vector_to_array(set->field_order); }
static inline  int srp_get_num_fields(sortable_ruleset *set){ return int_vector_size(set->field_order); }
static inline int srp_size(sortable_ruleset *set){ return rule_vector_size(set->rule_list); }

#endif
