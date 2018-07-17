#include "IntervalUtilities.h"

typedef struct weighted_pair{
	weighted_interval *first;
	int second;
} weighted_pair;

static int weighted_pair_compare(const void *lhs, const void *rhs);


int iu_count_pom(weighted_interval *wi, int second_field) {
	u_multiset *a_start = u_multiset_create();
	u_multiset *a_end = u_multiset_create();

	for (int i = 0; i < wi->num_rules; ++i) {
		u_multiset_insert(a_start, wi->rules[i]->fields[second_field].range[WI_LOW], 1);
		u_multiset_insert(a_end, wi->rules[i]->fields[second_field].range[WI_HIGH], 1);
	}
	int max_overlap = iu_get_max_overlap(a_start, a_end);

	u_multiset_destroy(a_start);
	u_multiset_destroy(a_end);

	return max_overlap;
}

weighted_interval * weighted_interval_create_i(rule **rules, const int NUM_RULES, unsigned int a, unsigned int b) {
	if (NUM_RULES == 0) {
		printf("ERROR: EMPTY RULE AND CONSTRUCT INTERVAL?\n");
		exit(0);
	}
	weighted_interval *wi = (weighted_interval*)safe_malloc(sizeof(weighted_interval));	
	wi->weight = 100000;	
	wi->rules = rules;
	wi->num_rules = NUM_RULES;
	wi->ival.first = a;
	wi->ival.second = b;
	iu_set_weight_by_size_plus_one(wi);
	wi->field = 0;
	return wi;
}

weighted_interval * weighted_interval_create_f(rule **rules, const int NUM_RULES, int field) {
	if (NUM_RULES == 0) {
		printf("ERROR: EMPTY RULE AND CONSTRUCT INTERVAL?\n");
		exit(0);
	}
	weighted_interval *wi = (weighted_interval*)safe_malloc(sizeof(weighted_interval));		
	wi->weight = 100000;	
	wi->rules = rules;
	wi->num_rules = NUM_RULES;
	wi->field = field;
	wi->ival.first = rules[0]->fields[field].range[WI_LOW];
	wi->ival.second = rules[0]->fields[field].range[WI_HIGH];
	iu_set_weight_by_size_plus_one(wi);
	return wi;
}

//!!!RETURNS DYNAMICALLY ALLOCATED MEMORY 
rule_vector * iu_redundancy_removal(rule_vector *rules) {
	bool *is_visited = (bool *)calloc(rule_vector_size(rules), sizeof(bool));
	rule_vector *out = rule_vector_init();
	rule **rules_a = rule_vector_to_array(rules);

	for (unsigned i = 0; i < rule_vector_size(rules); ++i)  {
		if (!is_visited[i]) {
			is_visited[i] = true;
			rule_vector_push_back(out, rules_a[i]);
			for (unsigned j = i + 1; j < rule_vector_size(rules); ++j) {
				if (iu_interval_is_identical(rules_a[i], rules_a[j])) {
					is_visited[j] = true;
				}
			}
		}
	}

	free(is_visited);

	return out;
}


bool iu_interval_is_identical(const rule *r1, const rule *r2) {
	for (int i = 0; i < r1->dim; ++i) {
		if (r1->fields[i].range[WI_LOW] != r2->fields[i].range[WI_LOW]) return false;
		if (r1->fields[i].range[WI_HIGH] != r2->fields[i].range[WI_HIGH]) return false;
	}
	return true;
}

int iu_get_max_overlap(u_multiset *lo, u_multiset *hi)
{
	if (u_multiset_size(lo) == 0) {
		return 0;
		//	cout << "error in GetMaxOverLap: lo size = 0 " << endl;
		//	exit(1);
	}
	unsigned *start_time, *end_time;

	start_time = u_multiset_create_array(lo);
	end_time = u_multiset_create_array(lo);

	size_t i = 0, j = 0;
	int max_overlap = -1;
	int current_overlap = 0;
	while (i < u_multiset_size(lo) && j < u_multiset_size(hi))
	{
		if (start_time[i] <= end_time[j])
		{
			current_overlap++;
			max_overlap = current_overlap > max_overlap ? current_overlap : max_overlap;
			i++;
		} else
		{
			current_overlap--;
			j++;
		}
	}

	free(start_time);
	free(end_time);

	return max_overlap;
}


void fast_mwis_intervals(light_weighted_interval **I, const int NUM_INTERVALS, int_vector **max_set, int *max){
	//I is already sorted 
	int temp_max = 0;
	int max_MIS = 0;
	*max_set = int_vector_initc(NUM_INTERVALS);
	int last_interval = 0;
	int *chi = (int *)calloc(NUM_INTERVALS, sizeof(int));



	end_point *endpoints = (end_point *)safe_malloc(sizeof(end_point) * NUM_INTERVALS * 2);
	for (size_t i = 0; i < NUM_INTERVALS * 2; i += 2) {
		endpoints[i].val = I[i/2]->a;
		endpoints[i].is_right_end = 0;
		endpoints[i].id = i/2;
		endpoints[i+1].val = I[i/2]->b;
		endpoints[i+1].is_right_end = 1;
		endpoints[i+1].id = i/2;
	}

	qsort(endpoints, NUM_INTERVALS * 2, sizeof(end_point), end_point_less_than);
	int index_start_no_change = 0, index_end_no_change = 0;
	for (size_t i = 0; i < NUM_INTERVALS * 2 - 1; ++i) {
		if (endpoints[i].val != endpoints[i + 1].val) {
			index_end_no_change = i;
			if (index_start_no_change != index_end_no_change) {
				double add_right_endpoint = 0.0001;
				double subtract_left_endpoint = -0.0001;
				for (int k = index_start_no_change; k <= index_end_no_change; ++k) {
					if (endpoints[k].is_right_end) {
						endpoints[k].val += add_right_endpoint;
						add_right_endpoint += 0.0001;
					} else {
						endpoints[k].val += subtract_left_endpoint;
						subtract_left_endpoint -= 0.0001;
					}
				}
			}
			index_start_no_change = i + 1;
		}
	}
	if (index_start_no_change != NUM_INTERVALS * 2 - 1) {
		double add_right_endpoint = 0.0001;
		double subtract_left_endpoint = -0.0001;
		for (size_t k = index_start_no_change; k <= NUM_INTERVALS * 2 - 1; ++k) {
			if (endpoints[k].is_right_end) {
				endpoints[k].val += add_right_endpoint;
				add_right_endpoint += 0.0001;
			} else {
				endpoints[k].val += subtract_left_endpoint;
				subtract_left_endpoint -= 0.0001;
			}
		}
	}
	qsort(endpoints, NUM_INTERVALS * 2, sizeof(end_point), end_point_less_than);

	end_point *e = NULL;
	for (size_t i = 0; i < NUM_INTERVALS * 2; ++i) {
		e = &endpoints[i];
		if (!e->is_right_end) {
			chi[e->id] = temp_max + lwi_get_weight(I[e->id]);
		} else {
			if (chi[e->id] > temp_max) {
				temp_max = chi[e->id];
				last_interval = e->id;
			}
		}
	}

	int_vector_push_back((*max_set), last_interval);
	max_MIS = temp_max;
	temp_max = temp_max - lwi_get_weight(I[last_interval]);
	for (int j = last_interval - 1; j >= 0; --j) {
		if (chi[j] == temp_max && lwi_get_high(I[j]) < lwi_get_low(I[last_interval])) {
			int_vector_push_back((*max_set), j);
			temp_max = temp_max - lwi_get_weight(I[j]);
			last_interval = j;
		}
	}
	
	free(chi);
	free(endpoints);

	*max = max_MIS;
}

void mwis_intervals_rules(rule_vector *I, int x, int_vector **max_set, int *max) {
	// create an weighted interval from rules
	weighted_interval **vc = (weighted_interval**)safe_malloc(sizeof(weighted_interval*) * rule_vector_size(I));
	rule ** rules = rule_vector_to_array(I);

	for(size_t i = 0; i < rule_vector_size(I); ++i){
		rule **temp_rules = (rule **)safe_malloc(sizeof(rule *));
		temp_rules[0] = rules[i];
		vc[i] =	weighted_interval_create_f(temp_rules, 1, x);
	}
	
	mwis_intervals(vc, rule_vector_size(I), max_set, max);

	for(size_t i = 0; i < rule_vector_size(I); ++i){
		weighted_interval_destroy(vc[i]);
	}
	free(vc);
}

//!!!DYNAMICALLY ALLOCATES MEMORY FOR THE PARAMETER "out"
void fast_create_unique_interval(interval *rules, const int NUM_RULES, light_weighted_interval ***out, int *num_intervals) {

	int count = 1;
	*out = (light_weighted_interval **)safe_malloc(sizeof(light_weighted_interval *) * NUM_RULES);
	interval *current_interval = &rules[0];
	*num_intervals = 0;

	for (int i = 1; i < NUM_RULES; ++i) {
		if (interval_equals(&rules[i], current_interval)) {
			count++;
		}
		else {
			(*out)[*num_intervals] = lwi_create();
			(*out)[*num_intervals]->a = current_interval->a;
			(*out)[*num_intervals]->b = current_interval->b;
			(*out)[*num_intervals]->w = count + 1;
			(*num_intervals)++;
			current_interval = &rules[i];
			count = 1;
		}
		if (i == NUM_RULES - 1) {
			(*out)[*num_intervals] = lwi_create();
			(*out)[*num_intervals]->a = current_interval->a;
			(*out)[*num_intervals]->b = current_interval->b;
			(*out)[*num_intervals]->w = count + 1;
			(*num_intervals)++;
		}
	}

}


//!!!DYNAMICALLY ALLOCATES MEMORY FOR THE PARAMETER "intervals"
void create_unique_interval(rule_vector *rules, int field, weighted_interval ***intervals, int *num_intervals) {
	//sort partition by field j

	//Build an array of intervals with the rules and sort
	int num_rules = rule_vector_size(rules);

	interval *sorted_rules = (interval *)safe_malloc(sizeof(interval) * num_rules);
	rule **rules_array = rule_vector_to_array(rules);


	for(size_t i = 0; i < num_rules; ++i){
		build_interval(rules_array[i]->fields[field].range[WI_LOW], rules_array[i]->fields[field].range[WI_HIGH], i, &sorted_rules[i]);
	}

	qsort(sorted_rules, num_rules, sizeof(interval), interval_less_than);
	
	interval *cur_int = &sorted_rules[0];

	//2D array of rule pointers to build the WeightedIntervals
	rule ***temp_rules = (rule ***)safe_malloc(sizeof(rule **) * num_rules);

	//Array of the number of rules contained in each weighted_interval
	int *count = (int *)safe_malloc(sizeof(int) * num_rules);

	//Total number of WeightedIntervals
	*num_intervals = 0;

	//Start one past the current interval for each iteration
	for(int i = 0; i < num_rules; ++i){
		
		count[*num_intervals] = 1;
	
		while(i + count[*num_intervals] < num_rules && interval_equals(cur_int, &sorted_rules[i + count[*num_intervals]])){
			count[*num_intervals]++;
		}
		
		temp_rules[*num_intervals] = (rule **)safe_malloc(sizeof(rule *) * count[*num_intervals]);	
			
		for(int j = 0, k = i; j < count[*num_intervals]; ++j, ++k){
			temp_rules[*num_intervals][j] = rules_array[sorted_rules[k].id];
		}

		i += count[*num_intervals] - 1;


		//i is now one more than the index of the last equal interval
		cur_int = &sorted_rules[i + 1];
		(*num_intervals)++;
	}


	//Each array is passed to weighted_interval_create_f and is deallocated later
	*intervals = (weighted_interval **)safe_malloc(sizeof(weighted_interval *) * (*num_intervals));
	for(int i = 0; i < *num_intervals; ++i){
		(*intervals)[i] = weighted_interval_create_f(temp_rules[i], count[i], field);
	}

	free(sorted_rules);
	free(temp_rules);
	free(count);	
}

static int weighted_pair_compare(const void *lhs, const void *rhs){
	weighted_pair *wlhs = (weighted_pair *)lhs;
	weighted_pair *wrhs = (weighted_pair *)rhs;
	return (int)wi_get_high(wlhs->first) - (int)wi_get_high(wrhs->first);
}

void mwis_intervals(weighted_interval **I, const int NUM_INTERVALS, int_vector **max_set, int *max) {
	const int LOW = 0;
	const int HIGH = 1;
	//need to sort id by endpoints
	int temp_max = 0;
	int max_MIS = 0;
	*max_set = int_vector_initc(NUM_INTERVALS);
	int last_interval = 0;
	int *chi = (int *)calloc(NUM_INTERVALS, sizeof(int));

	weighted_pair *sorted_I = (weighted_pair *)safe_malloc(sizeof(weighted_pair) * NUM_INTERVALS);
	for (int i = 0; i < NUM_INTERVALS; ++i) {
		sorted_I[i].first = I[i];
		sorted_I[i].second = i;
	}

	qsort(sorted_I, NUM_INTERVALS, sizeof(weighted_pair), weighted_pair_compare);

	end_point *endpoints = (end_point *)safe_malloc(sizeof(end_point) * NUM_INTERVALS * 2);
	for (int i = 0; i < NUM_INTERVALS * 2; i += 2) {
		endpoints[i].val = wi_get_low(I[i/2]);
		endpoints[i].is_right_end = LOW;
		endpoints[i].id = i/2;
		endpoints[i+1].val = wi_get_high(I[i/2]);
		endpoints[i+1].is_right_end = HIGH;
		endpoints[i+1].id = i/2;
	}

	qsort(endpoints, NUM_INTERVALS * 2, sizeof(end_point), end_point_less_than);

	//add perturbation to make points unique

	int index_start_no_change = 0, index_end_no_change = 0;
	for (size_t i = 0; i < (NUM_INTERVALS * 2) - 1; ++i) {
		if (endpoints[i].val != endpoints[i + 1].val) {
			index_end_no_change = i;
			if (index_start_no_change != index_end_no_change) {
				double add_right_endpoint = 0.0001;
				double subtract_left_endpoint = -0.0001;
				for (int k = index_start_no_change; k <= index_end_no_change; ++k) {
					if (endpoints[k].is_right_end) {
						endpoints[k].val += add_right_endpoint;
						add_right_endpoint += 0.0001;
					} else {
						endpoints[k].val += subtract_left_endpoint;
						subtract_left_endpoint -= 0.0001;
					}
				}
			}
			index_start_no_change = i + 1;
		}
	}
	if (index_start_no_change != (NUM_INTERVALS * 2) - 1) {
		double add_right_endpoint = 0.0001;
		double subtract_left_endpoint = -0.0001;
		for (int k = index_start_no_change; k <= (NUM_INTERVALS * 2) - 1; ++k) {
			if (endpoints[k].is_right_end) {
				endpoints[k].val += add_right_endpoint;
				add_right_endpoint += 0.0001;
			} else {
				endpoints[k].val += subtract_left_endpoint;
				subtract_left_endpoint -= 0.0001;
			}
		}
	}
	qsort(endpoints, NUM_INTERVALS * 2, sizeof(end_point), end_point_less_than);

	end_point *e = NULL;

	for (int i = 0; i < NUM_INTERVALS * 2; ++i) {
		e = &endpoints[i];
		if (!e->is_right_end) {
			chi[e->id] = temp_max + wi_get_weight(sorted_I[e->id].first);
		} else {
			if (chi[e->id] > temp_max) {
				temp_max = chi[e->id];
				last_interval = e->id;
			}
		}
	}

	int_vector_push_back(*max_set, sorted_I[last_interval].second);
	max_MIS = temp_max;
	temp_max = temp_max - wi_get_weight(sorted_I[last_interval].first);
	for (int j = last_interval - 1; j >= 0; --j) {
		if (chi[j] == temp_max && wi_get_high(sorted_I[j].first) < wi_get_low(sorted_I[last_interval].first)) {
			int_vector_push_back(*max_set, sorted_I[j].second);
			temp_max = temp_max - wi_get_weight(sorted_I[j].first);
			last_interval = j;
		}
	}

	*max = max_MIS;

	free(sorted_I);
	free(chi);
	free(endpoints);
	
}
