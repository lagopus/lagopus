#ifndef  SORTABLE_H
#define  SORTABLE_H
#include "ElementaryClasses.h"
#include "IntervalUtilities.h"
#include "rule_vector.h"
#include "int_vector.h"

typedef struct sortable_ruleset{
	rule_vector *rule_list;
	int_vector *field_order;
} sortable_ruleset;

typedef rule_vector part;

const static int SRLOW = 0 , SRHIGH = 1;
bool is_bucket_really_sortable( sortable_ruleset *b);

/*For GrInd */
//This function takes O(d^2 nlog n) time.
void sortable_ruleset_partitioning_gfs( rule_vector *rules, sortable_ruleset ***partitions, int *num_partitions);
void greedy_field_selection( rule_vector *in_rules, rule_vector **out_rules, int_vector **current_field);

void fast_greedy_field_selection_two_iterations( rule_vector *rules, bool *size, int_vector **current_field);

void fast_greedy_field_selection( rule_vector *in_rules, rule_vector **out_rules, int_vector **current_field);
void fast_greedy_field_selection_for_adaptive( rule_vector *rules, bool *size, int_vector **current_field);

void get_field_order_by_rule( rule *r, int **field_order, int *num_fields);
void adaptive_incremental_insertion( rule_vector *rules, sortable_ruleset ***partitions, int *num_partitions,  int);

/*For MISF*/
//Warning: this function takes O(d!dn log n) time.
 void maximum_independent_set_partitioning( rule_vector *rules, sortable_ruleset ***partitions, int *num_partitions);

/*For NON*/
//Warning: this function takes O(n^d) time.
 int  copmute_max_intersection( rule_vector *rules);

//Adaptation of http://www.comp.dit.ie/rlawlor/Alg_DS/sorting/quickSort.c
static inline int sr_partition( int *a, int *idx, int l, int r) {
   int i, j, k;
   int pivot = a[l];
   int t;
   i = l; j = r+1;
		
   while( 1)
   {
   	do ++i; while(  i <= r && a[i] <= pivot);
   	do --j; while( a[j] > pivot );
   	if( i >= j ) break;
	if(a[i] != a[j]){
	   	t = a[i]; a[i] = a[j]; a[j] = t;
		k = idx[i]; idx[i] = idx[j]; idx[j] = k;
	}
   }
   if(a[l] != a[j]){
	   t = a[l]; a[l] = a[j]; a[j] = t;
	   k = idx[l]; idx[l] = idx[j]; idx[j] = k;
   }
   return j;
}

static inline void sr_quick_sort( int *a, int *idx, int l, int r)
{
   int j = 0;

   if( l < r ) 
   {
   	// divide and conquer
        j = sr_partition( a, idx, l, r);
       sr_quick_sort( a, idx, l, j-1);
       sr_quick_sort( a, idx, j+1, r);
   }
	
}

static inline int * sort_indexes( int *v, const int SIZE) {

	// initialize original index locations
	int *idx = (int *)safe_malloc(SIZE * sizeof(int));
	for (int i = 0; i != SIZE; ++i) idx[i] = i;
	
	// sort indexes based on comparing values in v
	sr_quick_sort(v, idx, 0, SIZE - 1);

	return idx;
}


/*Helper For GrInd */ 
void is_this_partition_sortable( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, bool *is_sortable);
void is_entire_partition_sortable( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, bool *is_sortable);
void mwis_on_partition( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, int *max_MIS);
void mwis_on_entire_partition( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, int *sum_weight);
void best_field_and_configuration(rule_vector ***all_partitions, int *num_partitions, int_vector *current_field, int num_fields, bool do_not_delete);

void fast_mwis_on_partition( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, int *max_MIS);
void fast_mwis_on_entire_partition( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, int *sum_weight);
void fast_best_field_and_configuration(rule_vector ***all_partitions, int *num_partitions, int_vector *current_field, int num_fields, bool do_not_delete);


/*Helper For MISF*/
void maximum_independent_set_all_fields( rule_vector *in_rules, rule_vector **out_rules, int **field_order, int *num_fields);
rule_vector *  maximum_independent_set_given_field( rule_vector *rules,  int *field_order,  int NUM_FIELDS);
weighted_interval* maximum_independent_set_given_field_recursion( rule_vector *rules,  int *field_order,  int NUM_FIELDS, int depth, unsigned int a, unsigned int b);


/*Helper For NON*/
//Warning: this function takes O(n^d) time.
//field order is the suggested order to explore
int compute_max_intersection_recursive( rule_vector *rules, int field_depth,  int *field_order,  int NUM_FIELDS);

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


static inline bool srp_insert_rule(sortable_ruleset *set, rule *r) {
	//This function is for debugging purpose.
	rule_vector *temp_rule_list = rule_vector_initv(set->rule_list);
	rule_vector_push_back(temp_rule_list, r);
	int_vector *temp_field_order = int_vector_initv(set->field_order);
	sortable_ruleset *temp_set = sortable_ruleset_init(temp_rule_list, temp_field_order);

	if (is_bucket_really_sortable(temp_set)) {
		sortable_ruleset_destroy(temp_set);
		rule_vector_push_back(set->rule_list, r);
		return true;
	}
	sortable_ruleset_destroy(temp_set);
	return false;
}

static inline  rule_vector* srp_get_rule(sortable_ruleset *set){ return set->rule_list; }
static inline  int* srp_get_field_ordering(sortable_ruleset *set){ return int_vector_to_array(set->field_order); }
static inline  int srp_get_num_fields(sortable_ruleset *set){ return int_vector_size(set->field_order); }

static inline int srp_size(sortable_ruleset *set)
{
	return rule_vector_size(set->rule_list);
}

static inline void srp_print(sortable_ruleset *set)
{
	for (int i = 0; i < rule_vector_size(set->rule_list); ++i) {
		rule * r = rule_vector_get(set->rule_list, i);
		for (int j = 0; j < int_vector_size(set->field_order); ++j) {
			printf("[%u, %u]", r->range[int_vector_get(set->field_order, j)][low_dim], r->range[int_vector_get(set->field_order, j)][high_dim]);
		}
		printf("->%lld\n", r->priority);
	}
}

static inline sortable_ruleset * sortable_ruleset_assign(sortable_ruleset *a,  sortable_ruleset *b)
{
	// Check for self assignment
	if (a != b)
	{
		rule_vector_free(a->rule_list);
		int_vector_free(a->field_order);
		a->rule_list = rule_vector_initv(b->rule_list);
		a->field_order = int_vector_initv(b->field_order);
	}

	return a;
}
#endif
