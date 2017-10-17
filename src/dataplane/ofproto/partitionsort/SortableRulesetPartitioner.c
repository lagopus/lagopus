#include "SortableRulesetPartitioner.h"
#include "u_multiset.h"
#include <assert.h>

void greedy_field_selection_two_iterations( rule_vector *rules, bool *size, int_vector **current_field);
void srp_shuffle(int *array, size_t n);
void insert_rule_into_all_bucket( rule *r, sortable_ruleset **b, int *num_partitions,  int THRESHOLD);
int  compute_max_intersection( rule_vector *rules);
bool next_permutation(int *array,  int NUM_ELEMENTS);

//!!!Dynamic memory allocated for parameter "partitions" 
void is_this_partition_sortable( rule_vector *apartition, int current_field, rule_vector ***partitions, int *num_partitions, bool *is_sortable){
	weighted_interval **wi = NULL;//DA
	create_unique_interval(apartition, current_field, &wi, num_partitions);

	u_multiset *low, *hi;
	low = u_multiset_create();
	hi = u_multiset_create();

	for (int i = 0; i < *num_partitions; ++i) {
		u_multiset_insert(low, wi_get_low(wi[i]), 1);
		u_multiset_insert(hi, wi_get_high(wi[i]), 1);
	}

	*is_sortable = (bool)iu_get_max_overlap(low, hi);

	u_multiset_destroy(low);
	u_multiset_destroy(hi);

	*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_partitions));
	for (int i = 0; i < *num_partitions; ++i) {
		rule_vector *temp_partition_rule = rule_vector_init();

		rule **wi_rules = wi_get_rules(wi[i]);
		for (int j = 0; j < wi_get_num_rules(wi[i]); ++j) {
			rule_vector_push_back(temp_partition_rule, wi_rules[j]);
		}
		(*partitions)[i] = temp_partition_rule;
	}

	for(int i = 0; i < *num_partitions; ++i){
		weighted_interval_destroy(wi[i]);
	}
	free(wi);
	
}
//!!!Dynamically allocates memory for parameter "partitions"
void is_entire_partition_sortable( rule_vector **all_partitions,  int NUM_PARTITIONS, int current_field, rule_vector ***partitions, int *num_parts, bool *is_sortable){
	rule_vector ***all_temp_partitions = (rule_vector ***)safe_malloc(sizeof(rule_vector **) * NUM_PARTITIONS);
	int *all_temp_num_parts = (int *)safe_malloc(sizeof(int) * NUM_PARTITIONS);
	*num_parts = 0;
	*is_sortable = false;
	for (int i = 0; i < NUM_PARTITIONS; ++i) {

		bool is_sortable = false;
		rule_vector **temp_partition = NULL;//DA (containing rule_vectors that are dynamically allocated)
		int temp_num_parts = 0;
		is_this_partition_sortable(all_partitions[i], current_field, &temp_partition, &temp_num_parts, &is_sortable);

		if (!is_sortable){
			*partitions = (rule_vector **)safe_malloc(sizeof(rule_vector *) * (*num_parts));
			int j = 0;
			for(int k = 0; j < *num_parts && k < i; ++k){
				for(int l = 0; l < all_temp_num_parts[k]; ++l){
					(*partitions)[j] = all_temp_partitions[k][l];
					j++;
				}
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
	int i = 0;
	for(int j = 0; i < *num_parts && j < NUM_PARTITIONS; ++j){
		for(int k = 0; k < all_temp_num_parts[j]; ++k){
			(*partitions)[i] = all_temp_partitions[j][k];
			i++;
		}
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
		rule_vector *temp_partition_rule = rule_vector_init();
		rule **wi_rules = wi_get_rules(wi[i]);
		for (int j = 0; j < wi_get_num_rules(wi[i]); ++j) {
			rule_vector_push_back(temp_partition_rule, wi_rules[j]);
		}
		(*partitions)[a] = temp_partition_rule;
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
		for(int k = 0; k < all_temp_num_parts[j]; ++k){
			(*partitions)[i] = all_temp_partitions[j][k];
			i++;
		}
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
	for (size_t i = 0; i < rule_vector_size(current_rules); ++i) current_rules_array[i]->id = i;
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

void insert_rule_into_all_bucket( rule *r, sortable_ruleset **b, int *num_partitions,  int THRESHOLD) {
		for (int i = 0; i < *num_partitions; ++i) {
			if (srp_insert_rule(b[i], r)) {
				if (srp_size(b[i]) < THRESHOLD) {
					//reruct 
					rule_vector *temp_rules = srp_get_rule(b[i]);
					rule_vector *new_rules = NULL;
					int_vector *new_field = NULL;

					greedy_field_selection(temp_rules, &new_rules, &new_field);

					sortable_ruleset_destroy(b[i]);
					b[i] = sortable_ruleset_init(new_rules, new_field);
				}
				return;
			}
		}
		int *field_order0 = NULL;
		int num_fields = 0;
		
		if (THRESHOLD == 0) {
			
			field_order0 = (int *)safe_malloc(sizeof(int) * r->dim);			

			for(int i = 0; i < r->dim; ++i){ field_order0[i] = i; }
			srp_shuffle(field_order0, r->dim);
		}
		else {
			get_field_order_by_rule(r, &field_order0, &num_fields);
		}
		
		b[(*num_partitions)++] = sortable_ruleset_init(rule_vector_initr(&r, 1), int_vector_inita(field_order0, r->dim));
}

void adaptive_incremental_insertion( rule_vector *rules, sortable_ruleset ***partitions, int *num_partitions,  int THRESHOLD) {
	*partitions = (sortable_ruleset **)safe_malloc(sizeof(sortable_ruleset *) * (rule_vector_size(rules)));//At most, there will be the same number of partitions as rules
	*num_partitions = 0;
	rule **rules_array = rule_vector_to_array(rules);

	for (int i = 0; i < rule_vector_size(rules); ++i) {
		insert_rule_into_all_bucket(rules_array[i], *partitions, num_partitions, THRESHOLD);
	}
	
	*partitions = (sortable_ruleset **)realloc(*partitions, sizeof(sortable_ruleset *) * (*num_partitions));

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
		 unsigned int length = r->range[imod5][SRHIGH] - r->range[imod5][SRLOW] + 1;
		 if (length == 1) rank[i] = 0;
		 else if (length > 0 && length < (1 << 31)) rank[i] = 1;
		 else if (length == 0) rank[i] = 3;
		 else rank[i] = 2;
		} else if (imod5 == 2 || imod5 == 3) {
		 //Port
		 unsigned int length = r->range[imod5][SRHIGH] - r->range[imod5][SRLOW] + 1;
		 if (length == 1) rank[i] = 0;
		 else if (length < (1 << 15)) rank[i] = 1;
		 else if (length < (1 << 16)) rank[i] = 2;
		 else rank[i] = 3;
		} else {
		 //Protocol
		 unsigned int length = r->range[imod5][SRHIGH] - r->range[imod5][SRLOW] + 1;
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

int  compute_max_intersection( rule_vector *rules){
	if (rule_vector_size(rules) == 0) printf("Warning SortableRulesetPartitioner::compute_max_intersection: Empty ruleset\n");
	//auto  result = SortableRulesetPartitioner::greedy_field_selection(ruleset);
	int field_order[5] = { 0,1,2,3,4 };//result.second;
	//std::reverse(begin(field_order), end(field_order));
	return compute_max_intersection_recursive(rules, rule_vector_get(rules, 0)->dim - 1, field_order, 5);
}

typedef struct point_pair{
	unsigned int first;
	unsigned int second;
} point_pair;

int point_pair_compare(const void *l, const void *r);
int point_pair_compare(const void *l, const void *r){
	point_pair *lhs = (point_pair *)l;
	point_pair *rhs = (point_pair *)r;
	if(lhs->first == rhs->first){
		return lhs->second - rhs->second;
	}
	else{
		return lhs->first - rhs->first;
	}
}

int simple_compare_u(const void *l, const void *r);
int simple_compare_u(const void *l, const void *r){
	unsigned int *lhs = (unsigned int *)l;
	unsigned int *rhs = (unsigned int *)r;
	if(*lhs < *rhs){
		return -1;
	}
	else if(*rhs < *lhs){
		return 1;
	}
	else{
		return 0;
	}
}

int compute_max_intersection_recursive( rule_vector *rules, int field_depth,  int *field_order,  int NUM_FIELDS){

	int rules_size = rule_vector_size(rules);

	if (rules_size == 0) return 0;
	if (field_depth <= 0) {
		u_multiset *Astart = u_multiset_create();
		u_multiset *Aend = u_multiset_create();
		rule_vector *rules_rr = iu_redundancy_removal(rules);
		rule **rules_rr_array = rule_vector_to_array(rules_rr);
		for (int i = 0; i < rule_vector_size(rules_rr); ++i){
			u_multiset_insert(Astart, rules_rr_array[i]->range[field_order[field_depth]][SRLOW], 1);
			u_multiset_insert(Aend, rules_rr_array[i]->range[field_order[field_depth]][SRHIGH], 1);
		}
		int max_olap = iu_get_max_overlap(Astart, Aend);

		u_multiset_destroy(Astart);
		u_multiset_destroy(Aend);
		rule_vector_free(rules_rr);

		return max_olap;
	}


	point_pair *start_pointx = (point_pair *)safe_malloc(sizeof(point_pair) * rules_size);
	point_pair *end_pointx = (point_pair *)safe_malloc(sizeof(point_pair) * rules_size);
	unsigned int *queries = (unsigned int *)safe_malloc(sizeof(unsigned int) * 2 * rules_size);


	rule **rules_array = rule_vector_to_array(rules);
	for (int i = 0; i< rules_size; ++i) {
		start_pointx[i].first = rules_array[i]->range[field_order[field_depth]][SRLOW];
		start_pointx[i].second = i;
		end_pointx[i].first = rules_array[i]->range[field_order[field_depth]][SRHIGH];
		end_pointx[i].second = i;
		queries[i] = rules_array[i]->range[field_order[field_depth]][SRLOW];
		queries[i] = rules_array[i]->range[field_order[field_depth]][SRHIGH];
	}

	qsort(start_pointx, rules_size, sizeof(point_pair), point_pair_compare);
	qsort(end_pointx, rules_size, sizeof(point_pair), point_pair_compare);
	qsort(queries, rules_size * 2, sizeof(unsigned int), simple_compare_u);

	int i_s = 0;
	int i_e = 0;
	int max_POM = 0;


	rule_vector *rule_this_loop = rule_vector_init();
	int *index_rule_this_loop = (int *)safe_malloc(sizeof(int) * rules_size); //index by priority
	int *inverse_index_rule_this_loop = (int *)safe_malloc(sizeof(int) * rules_size);


	for(int i = 0; i < rules_size; ++i){
		index_rule_this_loop[i] = -1;
		inverse_index_rule_this_loop[i] = -1;

	}
	bool just_inserted = 0;
	bool just_deleted = 0;

	for (int i = 0; i < rules_size * 2; ++i) {
		//insert as long as start point is equal to qx
		if (rules_size != i_s) {
			while (queries[i] == start_pointx[i_s].first) {
				index_rule_this_loop[start_pointx[i_s].second] = (int)rule_vector_size(rule_this_loop);
				inverse_index_rule_this_loop[(int)rule_vector_size(rule_this_loop)] = start_pointx[i_s].second;
				rule_vector_push_back(rule_this_loop, rules_array[start_pointx[i_s].second]);
				i_s++;
				just_inserted = 1;
				if (rules_size == i_s) break;
			}
		}
		if (just_inserted) {
			int this_POM = compute_max_intersection_recursive(rule_this_loop, field_depth - 1, field_order, NUM_FIELDS);
			max_POM = this_POM > max_POM ? this_POM : max_POM;
		}

		if (rules_size != i_e){
		// delete as long as qx  > ie
			while (queries[i] == end_pointx[i_e].first) {
				int priority = end_pointx[i_e].second;
				if (index_rule_this_loop[priority] < 0) {
					printf("ERROR in OPTDecompositionRecursive: index_rule_this_loop[priority] < 0 \n");
					exit(1);
				}

				int index_to_delete = index_rule_this_loop[priority];
				int index_swap_in_table = inverse_index_rule_this_loop[(int)rule_vector_size(rule_this_loop) - 1];
				inverse_index_rule_this_loop[index_to_delete] = index_swap_in_table;
				index_rule_this_loop[index_swap_in_table] = index_to_delete;
				rule_vector_iswap(rule_this_loop, index_rule_this_loop[priority], rule_vector_size(rule_this_loop) - 1);
				rule_vector_pop(rule_this_loop);


				i_e++;
				just_deleted = 1;
				if (rules_size == i_e) break;
			}
		}
		if (just_deleted) {
			int this_POM = compute_max_intersection_recursive(rule_this_loop, field_depth - 1, field_order, NUM_FIELDS);
			max_POM = this_POM > max_POM ? this_POM : max_POM;
		}
	}

	free(start_pointx);
	free(end_pointx);
	free(queries);
	
	rule_vector_free(rule_this_loop);
	free(index_rule_this_loop);
	free(inverse_index_rule_this_loop);

	return max_POM;

}

void maximum_independent_set_partitioning( rule_vector *rules, sortable_ruleset ***partitions, int *num_partitions) {

	rule_vector *current_rules = rule_vector_initv(rules);
	rule ** current_rules_array = rule_vector_to_array(current_rules);

	*partitions = (sortable_ruleset **)safe_malloc(sizeof(sortable_ruleset *) * (rule_vector_size(rules)));//At most, there will be the same number of partitions as rules
	*num_partitions = 0;
	while (rule_vector_size(current_rules) != 0) {
		for (size_t i = 0; i < rule_vector_size(current_rules); ++i) current_rules_array[i]->id = i;

		rule_vector *part = NULL;
		int *field_order = NULL;
		int num_fields = 0;

		maximum_independent_set_all_fields(current_rules, &part, &field_order, &num_fields);

		rule **part_array = rule_vector_to_array(part);

		for (int i = 0; i < rule_vector_size(part); ++i)  {
			current_rules_array[part_array[i]->tag]->marked_delete = 1;
		}

		sortable_ruleset *sb = sortable_ruleset_init(part, int_vector_inita(field_order, num_fields));
 
		(*partitions)[(*num_partitions)++] = sb;
		
		current_rules_array = rule_vector_to_array(current_rules);
		for(int i = 0; i < rule_vector_size(current_rules); ++i){
			if(current_rules_array[i]->marked_delete){
				rule_vector_erase(current_rules, i);
				i--;
			}			
		}
		
	}

	*partitions = (sortable_ruleset **)realloc(*partitions, sizeof(sortable_ruleset *) * (*num_partitions));//Resize the array so no space is wasted

}

//Adapted implementation from c++ STL. Code found in gnu libstdc++ 4.7 and posted at https://stackoverflow.com/questions/11483060/stdnext-permutation-implementation-explanation
bool next_permutation(int *array,  int NUM_ELEMENTS){

        if (NUM_ELEMENTS < 2)
                return false;

        int i = NUM_ELEMENTS - 1;

	int begin = 0;
	int end = NUM_ELEMENTS;

        while (true)
        {
                int j = i;
                --i;

                if (array[i] < array[j])
                {
                        int k = NUM_ELEMENTS;

                        while (!(array[i] < array[--k]))
                                /* pass */;

                        int temp = array[i];
			array[i] = array[k];
			array[k] = temp;
			while ((j!=end)&&(j!=--end)) {
			    temp = array[j];
			    array[j] = array[end];
			    array[end] = temp;
			    ++j;
			}
                        return true;
                }

                if (i == begin)
                {
			int temp = 0;
			while ((begin!=end)&&(begin!=--end)) {
			    temp = array[begin];
			    array[begin] = array[end];
			    array[end] = temp;
			    ++begin;
			}
                        return false;
                }
        }
}

//!!!Dynamically allocates memory for parameters "out_rules" and "field_order"
void maximum_independent_set_all_fields( rule_vector *in_rules, rule_vector **out_rules, int **field_order, int *num_fields){
	*num_fields = rule_vector_get(in_rules, 0)->dim;	
	int *all_fields = (int *)safe_malloc(sizeof(int) * (*num_fields));
	for(int i = 0; i < *num_fields; ++i){ all_fields[i] = i; }
	rule_vector *best_so_far_rules = rule_vector_init();
	*field_order = (int *)safe_malloc(sizeof(int) * (*num_fields));
	do {
		rule_vector *vrules = maximum_independent_set_given_field(in_rules, all_fields, *num_fields);
		if (rule_vector_size(vrules) > rule_vector_size(best_so_far_rules)) {
			rule_vector_free(best_so_far_rules);
			best_so_far_rules = vrules;
			for(int i = 0; i < *num_fields; ++i){
				*field_order[i] = all_fields[i];
			}
		}
	} while (next_permutation(all_fields, *num_fields));

	*out_rules = best_so_far_rules;

	free(all_fields);
}

//!!!Dynamically allocates memory for return value
rule_vector *  maximum_independent_set_given_field( rule_vector *rules,  int *field_order,  int NUM_FIELDS){
	weighted_interval *wi = maximum_independent_set_given_field_recursion(rules, field_order, NUM_FIELDS, 0, 0, 0);
	rule_vector *max_set = rule_vector_initr(wi_get_rules(wi), wi_get_num_rules(wi));
	weighted_interval_destroy(wi);		
	return max_set;
}

//!!!Dynamically allocates memory for return value
weighted_interval* maximum_independent_set_given_field_recursion( rule_vector *rules,  int *field_order,  int NUM_FIELDS, int depth, unsigned int a, unsigned int b){
	if (a > b) {
		printf("Error: interval [a,b] where a > b???\n");
		exit(1);
	}
	int num_intervals = 0;
	weighted_interval **vwi = NULL;
	create_unique_interval(rules, field_order[depth], &vwi, &num_intervals);

	weighted_interval **vwi_opt = (weighted_interval **)safe_malloc(sizeof(weighted_interval *) * num_intervals);
	rule_vector *rules_mwis = rule_vector_init();

	int max = -1;
	int_vector *max_set = NULL;

	if (depth < NUM_FIELDS - 1) {
		for (int i = 0; i < num_intervals; ++i) {
			rule_vector *vwi_rules = rule_vector_initr(wi_get_rules(vwi[i]), wi_get_num_rules(vwi[i]));
			vwi_opt[i] = maximum_independent_set_given_field_recursion(vwi_rules, field_order, NUM_FIELDS, depth + 1, wi_get_low(vwi[i]), wi_get_high(vwi[i]));
			rule_vector_free(vwi_rules);
		}
		mwis_intervals(vwi_opt, num_intervals, &max_set, &max);

		int *max_set_array = int_vector_to_array(max_set);

		for (int i = 0; i < int_vector_size(max_set); ++i) {
			rule **vwi_rules = wi_get_rules(vwi_opt[max_set_array[i]]);
			for (int j = 0; j < wi_get_num_rules(vwi_opt[max_set_array[i]]); ++j) {
				rule_vector_push_back(rules_mwis, vwi_rules[j]);
			}
		}
		
		for(int i = 0; i < num_intervals; ++i){
			weighted_interval_destroy(vwi[i]);
			weighted_interval_destroy(vwi_opt[i]);
		}
		
	} else {
		mwis_intervals(vwi, num_intervals, &max_set, &max);

		int *max_set_array = int_vector_to_array(max_set);

		for (int i = 0; i < int_vector_size(max_set); ++i) {
			rule **vwi_rules = wi_get_rules(vwi[max_set_array[i]]);
			for (int j = 0; j < wi_get_num_rules(vwi[max_set_array[i]]); ++j) {
				rule_vector_push_back(rules_mwis, vwi_rules[j]);
			}
		}
		
		
		
		for(int i = 0; i < num_intervals; ++i){
			weighted_interval_destroy(vwi[i]);
		}
	}
	free(vwi);
	free(vwi_opt);
	int_vector_free(max_set);

	int num_rules = rule_vector_size(rules_mwis);
	rule **rules_mwis_copy = (rule **)safe_malloc(sizeof(rule *) * num_rules);
	rule **rules_mwis_a = rule_vector_to_array(rules_mwis);
	for(int i = 0; i < num_rules; ++i){
		rules_mwis_copy[i] = rules_mwis_a[i];
	}
	rule_vector_free(rules_mwis);

	return weighted_interval_create_i(rules_mwis_copy, num_rules, a, b);
}


int fast_interval_compare(const void *l, const void *r);
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
		rules_given_field[j].a = apart_rules[j]->range[current_field][0];
		rules_given_field[j].b = apart_rules[j]->range[current_field][1];
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
		for(int k = 0; k < all_temp_num_parts[j]; ++k){
			(*partitions)[i] = all_temp_partitions[j][k];
			i++;
		}
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
