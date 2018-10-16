#include <assert.h>
#include <string.h>

#include "SortableRulesetPartitioner.h"
#include "u_multiset.h"

int fast_interval_compare(const void *l, const void *r);

void greedy_field_selection_two_iterations( rule_vector *rules, bool *size, int_vector **current_field);
void srp_shuffle(int *array, size_t n);
static int sr_partition( int *a, int *idx, int l, int r);
static void sr_quick_sort( int *a, int *idx, int l, int r);

sortable_ruleset * sortable_ruleset_assign(sortable_ruleset *a,  sortable_ruleset *b)
{
	// Check for self assignment
	if (a != b)
	{
		rule_vector_clear(a->rule_list);
		int_vector_clear(a->field_order);
		rule_vector_push_range(a->rule_list, b->rule_list);
		int_vector_push_range(a->field_order, b->field_order);
	}

	return a;
}

//Adaptation of http://www.comp.dit.ie/rlawlor/Alg_DS/sorting/quickSort.c
static int sr_partition( int *a, int *idx, int l, int r) {
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

static void sr_quick_sort( int *a, int *idx, int l, int r)
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

int * sort_indexes( int *v, const int SIZE) {

	// initialize original index locations
	int *idx = (int *)safe_malloc(SIZE * sizeof(int));
	for (int i = 0; i != SIZE; ++i) idx[i] = i;
	
	// sort indexes based on comparing values in v
	sr_quick_sort(v, idx, 0, SIZE - 1);

	return idx;
}

//!!!Dynamic memory allocated for parameter "partitions" 
void is_this_partition_sortable( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, bool *is_sortable){
	weighted_interval **wi = NULL;//DA
	u_multiset *low, *hi;
	int i;
	//Putting variables here eliminates warnings

	create_unique_interval(apartition, current_field, &wi, num_partitions);

	low = u_multiset_create();
	hi = u_multiset_create();

	for (i = 0; i < *num_partitions; ++i) {
		u_multiset_insert(low, wi_get_low(wi[i]), 1);
		u_multiset_insert(hi, wi_get_high(wi[i]), 1);
	}

	*is_sortable = (bool)iu_get_max_overlap(low, hi);

	u_multiset_destroy(low);
	u_multiset_destroy(hi);

	*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_partitions));
	for (i = 0; i < *num_partitions; ++i) {
		(*partitions)[i] = rule_vector_initr(wi_get_rules(wi[i]), wi_get_num_rules(wi[i]));
	}

	for(i = 0; i < *num_partitions; ++i){
		weighted_interval_destroy(wi[i]);
	}
	free(wi);
	
}
//!!!Dynamically allocates memory for parameter "partitions"
void is_entire_partition_sortable( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, bool *is_sortable){
	rule_vector ***all_temp_partitions = (rule_vector ***)safe_malloc(sizeof(rule_vector **) * NUM_PARTITIONS);
	int *all_temp_num_parts = (int *)safe_malloc(sizeof(int) * NUM_PARTITIONS);
	int i;
	*num_parts = 0;
	*is_sortable = false;
	for (i = 0; i < NUM_PARTITIONS; ++i) {

		bool is_sortable = false;
		rule_vector **temp_partition = NULL;//DA (containing rule_vectors that are dynamically allocated)
		int temp_num_parts = 0;
		is_this_partition_sortable(all_partitions[i], current_field, &temp_partition, &temp_num_parts, &is_sortable);

		if (!is_sortable){
			*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_parts));
			int j = 0;
			for(int k = 0; j < *num_parts && k < i; ++k){
				memcpy(&((*partitions)[j]), all_temp_partitions[k], all_temp_num_parts[k] * sizeof(rule_vector *));
				j += all_temp_num_parts[k];
				free(all_temp_partitions[k]);
			}
			
			free(all_temp_partitions);
			free(all_temp_num_parts); 
			return;
		}

		*num_parts += temp_num_parts;
		all_temp_num_parts[i] = temp_num_parts;
		all_temp_partitions[i] = temp_partition;
	}

	*is_sortable = true;
	*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_parts));
	i = 0;
	for(int j = 0; i < *num_parts && j < NUM_PARTITIONS; ++j){
		memcpy(&((*partitions)[i]), all_temp_partitions[j], all_temp_num_parts[j] * sizeof(rule_vector *));
		i += all_temp_num_parts[j];
		free(all_temp_partitions[j]);
	}
	
	free(all_temp_partitions);
	free(all_temp_num_parts);
}

bool is_bucket_really_sortable( sortable_ruleset *b) {
	rule_vector **all_partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));
	all_partitions[0] = srp_get_rule(b);
	int num_partitions = 1;
	int *f = srp_get_field_ordering(b);
	bool is_sortable = false;
	for (int i = 0; i < srp_get_num_fields(b); ++i) {
		int current_num_parts = num_partitions;
		rule_vector **new_partitions = NULL;
		is_entire_partition_sortable(all_partitions, current_num_parts, f[i], &new_partitions, &num_partitions, &is_sortable);

		for(int j = 0; j < current_num_parts; ++j){
			rule_vector_free(all_partitions[j]);
		}
		free(all_partitions);

		if (!is_sortable){
			for(int j = 0; j < num_partitions; ++j){
				rule_vector_free(new_partitions[j]);
			}
			free(new_partitions); 
			return 0;
		}

		all_partitions = new_partitions;
	}
	for(int j = 0; j < num_partitions; ++j){
		rule_vector_free(all_partitions[j]);
	}
	free(all_partitions);
	return 1;
}

//!!!Dynamically allocates memory for parameter "partitions"
void mwis_on_partition( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, int *max_MIS){

	weighted_interval **wi = NULL;//DA
	int num_intervals = 0;
	create_unique_interval(apartition, current_field, &wi, &num_intervals);
	//printf("Field %d\n", current_field);

	int_vector *max_set = NULL;//DA
	*max_MIS = -1;
	mwis_intervals(wi, num_intervals, &max_set, max_MIS);//max_MIS set to proper value here

	int *max_set_array = int_vector_to_array(max_set);
	*num_partitions = int_vector_size(max_set);//num_partitions set to proper value here
	*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_partitions));	

	for (int a = 0; a < *num_partitions; ++a) {
		int i = max_set_array[a];
		(*partitions)[a] = rule_vector_initr(wi_get_rules(wi[i]), wi_get_num_rules(wi[i]));
	}

	int_vector_free(max_set);

	for(int i = 0; i < num_intervals; ++i){
		weighted_interval_destroy(wi[i]);
	}
	free(wi);
}

//!!!Dynamically allocates memory for parameter "partitions"
void mwis_on_entire_partition( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, int *sum_weight){
	rule_vector ***all_temp_partitions = (rule_vector ***)safe_malloc(sizeof(rule_vector **) * NUM_PARTITIONS);
	int *all_temp_num_parts = (int *)safe_malloc(sizeof(int) * NUM_PARTITIONS);
	*num_parts = 0;
	*sum_weight = 0;

	for (int i = 0; i < NUM_PARTITIONS; ++i) {
		rule_vector **temp_partition = NULL;//DA (containing rule_vectors that are dynamically allocated)
		int temp_num_parts = 0;
		int added_weight = 0;

		mwis_on_partition(all_partitions[i], current_field, &temp_partition, &temp_num_parts, &added_weight);
		
		all_temp_partitions[i] = temp_partition;
		all_temp_num_parts[i] = temp_num_parts;
		*num_parts += temp_num_parts;
		*sum_weight += added_weight;
	}

	*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_parts));
	int i = 0;
	for(int j = 0; i < *num_parts && j < NUM_PARTITIONS; ++j){
		memcpy(&((*partitions)[i]), all_temp_partitions[j], all_temp_num_parts[j] * sizeof(rule_vector *));
		i += all_temp_num_parts[j];
		free(all_temp_partitions[j]);
	}
	
	free(all_temp_partitions);
	free(all_temp_num_parts);

}

//!!!Dynamically allocates memory for parameter "partitions"
void best_field_and_configuration(rule_vector ***all_partitions, int *num_partitions, int_vector *current_field, int num_fields, bool do_not_delete)
{
	 
	rule_vector **best_new_partition = NULL;
	int best_num_partitions = 0;
	int best_so_far_mwis = -1;
	int current_best_field = -1;
	for (int j = 0; j < num_fields; ++j) {
		if (int_vector_find(current_field, j) == -1) {
			
			rule_vector **temp_partition = NULL;
			int temp_num_parts = 0;
			int temp_weight;

			mwis_on_entire_partition(*all_partitions, *num_partitions, j, &temp_partition, &temp_num_parts, &temp_weight);
			if (temp_weight >= best_so_far_mwis) {
				best_so_far_mwis = temp_weight;
				current_best_field = j;
				
				if(best_num_partitions != 0){
					for(int i = 0; i < best_num_partitions; ++i){
						rule_vector_free(best_new_partition[i]);
					}
					free(best_new_partition);
				}

				best_new_partition = temp_partition;
				best_num_partitions = temp_num_parts;
			}
			else{
				for(int i = 0; i < temp_num_parts; ++i){
					rule_vector_free(temp_partition[i]);
				}
				free(temp_partition);
			}
		}
	}
	if(!do_not_delete){
		for(int i = 0; i < *num_partitions; ++i){
			rule_vector_free((*all_partitions)[i]);
		}
	}
	free(*all_partitions);
	*all_partitions = best_new_partition;
	*num_partitions = best_num_partitions;
	int_vector_push_back(current_field, current_best_field);
}

//!!!Dynamically allocated parameter "current_field"
void greedy_field_selection_two_iterations( rule_vector *rules, bool *size, int_vector **current_field) {
	if (rule_vector_size(rules) == 0) {
		printf("Warning: greedy_field_selection rule size = 0\n ");
	}
	int num_fields = rule_vector_get(rules, 0)->dim;

	*current_field = int_vector_init();
	rule_vector **all_partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));

	all_partitions[0] = rules;
	int num_partitions = 1;
	for (int i = 0; i < 2; ++i) {
		best_field_and_configuration(&all_partitions, &num_partitions, *current_field, num_fields, i == 0 ? true : false);
	}

	int current_rules_size = 0;
	for(int i = 0; i < num_partitions; ++i){
		current_rules_size += rule_vector_size(all_partitions[i]);
		rule_vector_free(all_partitions[i]);
	}
	free(all_partitions);

	//fill in the rest of the field in order
	for (int j = 0; j < num_fields; ++j) {
		if (int_vector_find(*current_field, j) == -1) {
			int_vector_push_back(*current_field, j);
		}
	}

	*size = current_rules_size == rule_vector_size(rules);
}


void greedy_field_selection( rule_vector *in_rules, rule_vector **out_rules, int_vector **current_field)
{
	if (rule_vector_size(in_rules) == 0) {
		printf("Warning: greedy_field_selection rule size = 0\n ");
	}
	int num_fields = rule_vector_get(in_rules, 0)->dim;

	*current_field = int_vector_init();
	rule_vector **all_partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));

	all_partitions[0] = in_rules;
	int num_partitions = 1;
	for (int i = 0; i < num_fields; ++i) {
		best_field_and_configuration(&all_partitions, &num_partitions, *current_field, num_fields, i == 0 ? true : false);
	}

	*out_rules = rule_vector_init();
	for (int i = 0; i < num_partitions; ++i) {
		rule_vector_push_range(*out_rules, all_partitions[i]);
		rule_vector_free(all_partitions[i]);
	}
	free(all_partitions);

	//fill in the rest of the field in order
	for (int j = 0; j < num_fields; ++j) {
		if (int_vector_find(*current_field, j) == -1) {
			int_vector_push_back(*current_field, j);
		}
	}
}

void sortable_ruleset_partitioning_gfs( rule_vector *rules, sortable_ruleset ***partitions, int *num_partitions) {
	rule_vector *current_rules = rule_vector_initv(rules);
	rule ** current_rules_array = rule_vector_to_array(current_rules);

	*partitions = (sortable_ruleset **)safe_malloc(sizeof(sortable_ruleset *) * (rule_vector_size(rules)));//At most, there will be the same number of partitions as rules
	*num_partitions = 0;
	while (rule_vector_size(current_rules) != 0) {
		
		current_rules_array = rule_vector_to_array(current_rules);

		for (int i = 0; i < rule_vector_size(current_rules); ++i) current_rules_array[i]->tag = i;

		rule_vector *part = NULL;
		int_vector *field_order = NULL;

		greedy_field_selection(current_rules, &part, &field_order);

		rule **part_array = rule_vector_to_array(part);

		for (int i = 0; i < rule_vector_size(part); ++i)  {
			current_rules_array[part_array[i]->tag]->marked_delete = 1;
		}

		if (rule_vector_size(part) == 0) {
			printf("Warning rule_and_field_order.first = 0 in SortableRulesetPartitioning\n");
			exit(0);
		}

		sortable_ruleset *sb = sortable_ruleset_init(part, field_order);
 
		(*partitions)[(*num_partitions)++] = sb;
		
		current_rules_array = rule_vector_to_array(current_rules);
		for(int i = 0; i < rule_vector_size(current_rules); ++i){
			if(current_rules_array[i]->marked_delete){
				rule_vector_erase(current_rules, i);
				i--;
			}			
		}
		
	}

	rule_vector_free(current_rules);

	*partitions = (sortable_ruleset **)realloc(*partitions, sizeof(sortable_ruleset *) * (*num_partitions));//Resize the array so no space is wasted
}

/* Found on StackExchange, code by Ben Pfaff. https://stackoverflow.com/questions/6127503/shuffle-array-in-c
   Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void srp_shuffle(int *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; ++i) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

//!!!Dynamically allocated parameter "field_order"
void get_field_order_by_rule( rule *r, int **field_order, int *num_fields)
{
	//assume ClassBench Format
	int *rank = (int *)calloc(r->dim, sizeof(int));
	// 0 -> point, 1 -> shorter than half range, 2 -> longer than half range
	//assign rank to each field
	for (int i = 0; i < r->dim; ++i) {
		int imod5 = i % 5;
		if (imod5 == 0 || imod5 == 1) {
		 //IP
		 unsigned int length = r->fields[imod5].range[SRHIGH] - r->fields[imod5].range[SRLOW] + 1;
		 if (length == 1) rank[i] = 0;
		 else if (length > 0 && length < (1 << 31)) rank[i] = 1;
		 else if (length == 0) rank[i] = 3;
		 else rank[i] = 2;
		} else if (imod5 == 2 || imod5 == 3) {
		 //Port
		 unsigned int length = r->fields[imod5].range[SRHIGH] - r->fields[imod5].range[SRLOW] + 1;
		 if (length == 1) rank[i] = 0;
		 else if (length < (1 << 15)) rank[i] = 1;
		 else if (length < (1 << 16)) rank[i] = 2;
		 else rank[i] = 3;
		} else {
		 //Protocol
		 unsigned int length = r->fields[imod5].range[SRHIGH] - r->fields[imod5].range[SRLOW] + 1;
		 if (length == 1) rank[i] = 0;
		 else if (length < (1 << 7)) rank[i] = 1;
		 else if (length < 256) rank[i] = 2;
		 else rank[i] = 3;

		}
	}
	*field_order = sort_indexes(rank, r->dim);
	*num_fields = r->dim;
	free(rank);
}

int fast_interval_compare(const void *l, const void *r){
	interval *lhs = (interval *)l;
	interval *rhs = (interval *)r;
	if (lhs->b < rhs->b) {
		return -1;
	}
	if(lhs->a < rhs->a){
		return -1;
	}
	else if(rhs->a < lhs->a){
		return 1; 
	}
	else{
		return 0;
	}

}

//!!!Dynamically allocates memory for parameter "partitions"
void fast_mwis_on_partition( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, int *max_MIS) {

	int num_rules = rule_vector_size(apartition);
	rule **apart_rules = rule_vector_to_array(apartition);

	if (num_rules == 0) {
		*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));
		*num_partitions = 0;
		*max_MIS = 0;
		return;
	}

	interval *rules_given_field = (interval *)safe_malloc(sizeof(interval) * num_rules);

	for (int j = 0; j < num_rules; ++j) {
		rules_given_field[j].a = apart_rules[j]->fields[current_field].range[0];
		rules_given_field[j].b = apart_rules[j]->fields[current_field].range[1];
		rules_given_field[j].id = j;
	}
	qsort(rules_given_field, num_rules, sizeof(interval), fast_interval_compare);

	light_weighted_interval **wi = NULL;//DA
	int num_intervals = 0;

	fast_create_unique_interval(rules_given_field, num_rules, &wi, &num_intervals);

	int_vector *max_set = NULL;//DA

	fast_mwis_intervals(wi, num_intervals, &max_set, max_MIS);//Sets value of max_MIS

	*num_partitions = int_vector_size(max_set);
	*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_partitions));
	int *max_set_a = int_vector_to_array(max_set);

	for (int i = 0; i < *num_partitions; ++i) {

		rule_vector *temp_partition_rule = rule_vector_init();
		int *rule_indices = NULL;
		int num_indices = 0;

		lwi_get_rule_indices(wi[max_set_a[i]], &rule_indices, &num_indices);//This does not allocate any new memory

		for (int j = 0; j < num_indices; ++j) {
			
			rule_vector_push_back(temp_partition_rule, apart_rules[rules_given_field[rule_indices[j]].id]);
		}
		(*partitions)[i] = temp_partition_rule;
	}

	free(rules_given_field);
	for(int i = 0; i < num_intervals; ++i){
		lwi_destroy(wi[i]);
	}
	free(wi);
	int_vector_free(max_set);

}

void fast_mwis_on_entire_partition( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, int *sum_weight) {
	rule_vector ***all_temp_partitions = (rule_vector ***)safe_malloc(sizeof(rule_vector **) * NUM_PARTITIONS);
	int *all_temp_num_parts = (int *)safe_malloc(sizeof(int) * NUM_PARTITIONS);
	*num_parts = 0;
	*sum_weight = 0;

	for (int i = 0; i < NUM_PARTITIONS; ++i) {
		rule_vector **temp_partition = NULL;//DA (containing rule_vectors that are dynamically allocated)
		int temp_num_parts = 0;
		int added_weight = 0;

		fast_mwis_on_partition(all_partitions[i], current_field, &temp_partition, &temp_num_parts, &added_weight);
		
		all_temp_partitions[i] = temp_partition;
		all_temp_num_parts[i] = temp_num_parts;
		*num_parts += temp_num_parts;
		*sum_weight += added_weight;
	}

	if(*num_parts == 0){
		for(int i = 0; i < NUM_PARTITIONS; ++i){
			free(all_temp_partitions[i]);
		}
		free(all_temp_partitions);
		free(all_temp_num_parts);
		*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));
		return;
	}

	*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_parts));
	int i = 0;
	for(int j = 0; i < *num_parts && j < NUM_PARTITIONS; ++j){
		memcpy(&((*partitions)[i]), all_temp_partitions[j], sizeof(rule_vector *) * all_temp_num_parts[j]);
		i += all_temp_num_parts[j];
		free(all_temp_partitions[j]);
	}
	
	free(all_temp_partitions);
	free(all_temp_num_parts);

}

void fast_best_field_and_configuration(rule_vector ***all_partitions, int *num_partitions, int_vector *current_field, int num_fields, bool do_not_delete)
{
	rule_vector **best_new_partition = NULL;
	int best_num_partitions = 0;
	int best_so_far_mwis = -1;
	int current_best_field = -1;
	for (int j = 0; j < num_fields; ++j) {
		if (int_vector_find(current_field, j) == -1) {
			
			rule_vector **temp_partition = NULL;
			int temp_num_parts = 0;
			int temp_weight = 0;

			fast_mwis_on_entire_partition(*all_partitions, *num_partitions, j, &temp_partition, &temp_num_parts, &temp_weight);
			if (temp_weight >= best_so_far_mwis) {
				best_so_far_mwis = temp_weight;
				
				if(current_best_field != -1){
					for(int i = 0; i < best_num_partitions; ++i){
						rule_vector_free(best_new_partition[i]);
					}
					free(best_new_partition);
				}

				best_new_partition = temp_partition;
				best_num_partitions = temp_num_parts;
				current_best_field = j;
			}
			else{
				for(int i = 0; i < temp_num_parts; ++i){
					rule_vector_free(temp_partition[i]);
				}
				free(temp_partition);
			}
		}
	}
	if(!do_not_delete){
		for(int i = 0; i < *num_partitions; ++i){
			rule_vector_free((*all_partitions)[i]);
		}
	}
	free(*all_partitions);
	*all_partitions = best_new_partition;
	*num_partitions = best_num_partitions;
	int_vector_push_back(current_field, current_best_field);
}

void fast_greedy_field_selection_two_iterations( rule_vector *rules, bool *size, int_vector **current_field) {
	if (rule_vector_size(rules) == 0) {
		printf("Warning: greedy_field_selection rule size = 0\n ");
	}
	int num_fields = rule_vector_get(rules, 0)->dim;

	*current_field = int_vector_init();
	rule_vector **all_partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));

	all_partitions[0] = rules;
	int num_partitions = 1;
	for (int i = 0; i < 2; ++i) {
		fast_best_field_and_configuration(&all_partitions, &num_partitions, *current_field, num_fields, i == 0 ? true : false);
	}

	int current_rules_size = 0;
	for(int i = 0; i < num_partitions; ++i){
		current_rules_size += rule_vector_size(all_partitions[i]);
		rule_vector_free(all_partitions[i]);
	}
	free(all_partitions);

	//fill in the rest of the field in order
	for (int j = 0; j < num_fields; ++j) {
		if (int_vector_find(*current_field, j) == -1) {
			int_vector_push_back(*current_field, j);
		}
	}

	*size = current_rules_size == rule_vector_size(rules);
}

void fast_greedy_field_selection( rule_vector *in_rules, rule_vector **out_rules, int_vector **current_field)
{
	if (rule_vector_size(in_rules) == 0) {
		printf("Warning: greedy_field_selection rule size = 0\n ");
	}
	int num_fields = rule_vector_get(in_rules, 0)->dim;

	*current_field = int_vector_init();
	rule_vector **all_partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));

	all_partitions[0] = in_rules;
	int num_partitions = 1;
	for (int i = 0; i < num_fields; ++i) {
		fast_best_field_and_configuration(&all_partitions, &num_partitions, *current_field, num_fields, i == 0 ? true : false);
	}

	*out_rules = rule_vector_init();
	for (int i = 0; i < num_partitions; ++i) {
		rule_vector_push_range(*out_rules, all_partitions[i]);
		free(all_partitions[i]);
	}
	free(all_partitions);

	//fill in the rest of the field in order
	for (int j = 0; j < num_fields; ++j) {
		if (int_vector_find(*current_field, j) == -1) {
			int_vector_push_back(*current_field, j);
		}
	}
}

void fast_greedy_field_selection_for_adaptive( rule_vector *rules, bool *size, int_vector **current_field) {

	if (rule_vector_size(rules) == 0) {
		printf("Warning: greedy_field_selection rule size = 0\n ");
	}
	int num_fields = rule_vector_get(rules, 0)->dim;

	*current_field = int_vector_init();
	rule_vector **all_partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *));

	all_partitions[0] = rules;
	int num_partitions = 1;
	for (int i = 0; i < num_fields; ++i) {
		fast_best_field_and_configuration(&all_partitions, &num_partitions, *current_field, num_fields, i == 0 ? true : false);
	}

	*size = num_partitions == 0 ? 0 == rule_vector_size(rules) : rule_vector_size(all_partitions[0]) == rule_vector_size(rules);

	for (int i = 0; i < num_partitions; ++i) {
		free(all_partitions[i]);
	}
	free(all_partitions);

	//fill in the rest of the field in order
	for (int j = 0; j < num_fields; ++j) {
		if (int_vector_find(*current_field, j) == -1) {
			int_vector_push_back(*current_field, j);
		}
	}
}
