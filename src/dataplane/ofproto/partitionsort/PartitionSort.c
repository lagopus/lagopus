#include "PartitionSort.h"
#include "ll_vector.h"

void ps_delete_classifier_l(struct partition_sort *ps) {
	if(ps->num_rules == 0)//Already been deleted
		return;
	for (unsigned i = 0; i < ps->num_trees; ++i) {
		if(ps->mitrees[i] != NULL)
			optimized_mi_tree_destroy(ps->mitrees[i]);
	}
	free(ps->mitrees);
	free(ps->rules);
	free(ps->fields);
	ps->_rules_cap = 0;//Identifies if classifier has been built
}

void ps_construct_classifier_online_l(struct partition_sort *self, const rule **rules, const unsigned num_rules, const uint8_t *FIELDS, const int NUM_FIELDS)  {

	self->_tree_cap = 5;
	self->_rules_cap = num_rules == 0 ? 1 : num_rules;
	self->num_trees = 0;
	self->num_rules = 0;
	self->rules = (struct rt_pair *) malloc(num_rules * sizeof(struct rt_pair));
	self->mitrees = (optimized_mi_tree **)safe_malloc(sizeof(optimized_mi_tree *) * self->_tree_cap);
	for (unsigned i = 0; i < num_rules; ++i) {
		ps_insert_rule_s(self, rules[i]);//Initial classifier is not sorted, need to sort
	}

	self->fields = (uint8_t *)safe_malloc(sizeof(uint8_t) * NUM_FIELDS);
	memcpy(self->fields, FIELDS, sizeof(uint8_t) * NUM_FIELDS);
	self->num_fields = NUM_FIELDS;
}

void ps_construct_classifier_offline_l(struct partition_sort *self, const rule **rules, const unsigned num_rules, const uint8_t *FIELDS, const int NUM_FIELDS) { 

	
	self->num_rules = 0;
	self->rules = (struct rt_pair *) malloc(sizeof(struct rt_pair));
	self->_rules_cap = 1;

	sortable_ruleset **buckets = NULL;
	self->num_trees = 0;
	rule_vector *rules_v = rule_vector_initr(rules, num_rules);
	sortable_ruleset_partitioning_gfs(rules_v, &buckets, &self->num_trees);

	self->_tree_cap = self->num_trees;

	self->mitrees = (optimized_mi_tree **) safe_malloc(self->num_trees * sizeof( optimized_mi_tree *));
	for (int i = 0; i < self->num_trees; ++i)  {		
		self->mitrees[i] = optimized_mi_tree_init_sr(buckets[i]);
	}
	ps_insertion_sort_mitrees(self);

	rule_vector_free(rules_v);
	for(int i = 0; i < self->num_trees; ++i){
		sortable_ruleset_destroy(buckets[i]);
	}
	free(buckets);

	self->fields = (uint8_t *)safe_malloc(sizeof(uint8_t) * NUM_FIELDS);
	memcpy(self->fields, FIELDS, sizeof(uint8_t) * NUM_FIELDS);
	self->num_fields = NUM_FIELDS;
}

void ps_insert_rule(struct partition_sort *ps, const struct rule *one_rule) {

	for (unsigned i = 0; i < ps->num_trees; ++i)
	{
		bool priority_change = false;
		
		bool success = mit_try_insertion(ps->mitrees[i], one_rule, &priority_change);
		if (success) {
			
			if (priority_change) {
				ps_insertion_sort_mitrees(ps);
			}
			reconstruct_if_num_rules_leq(ps->mitrees[i], 10);
			if(ps->num_rules == ps->_rules_cap){
				ps->rules = (rt_pair *)realloc(ps->rules, sizeof(rt_pair) * ps->_rules_cap * 2);
				ps->_rules_cap *= 2;
			}
			ps->rules[ps->num_rules].first = one_rule;
			ps->rules[ps->num_rules].second = ps->mitrees[i];
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
	ps->num_rules++;

	if(ps->num_trees == ps->_tree_cap){
		ps->mitrees = (optimized_mi_tree **)realloc(ps->mitrees, sizeof(optimized_mi_tree *) * ps->_tree_cap * 2);
		ps->_tree_cap *= 2;
	}
	ps->mitrees[ps->num_trees++] = tree_ptr;  
	ps_insertion_sort_mitrees(ps);
}

//Inserts rule into ps->rules by priority in descending order
void ps_insert_rule_s(struct partition_sort *ps, const struct rule *one_rule) {

	for (unsigned i = 0; i < ps->num_trees; ++i)
	{
		bool priority_change = false;
		
		bool success = mit_try_insertion(ps->mitrees[i], one_rule, &priority_change);
		if (success) {
			
			if (priority_change) {
				ps_insertion_sort_mitrees(ps);
			}
			reconstruct_if_num_rules_leq(ps->mitrees[i], 10);
			if(ps->num_rules == ps->_rules_cap){
				ps->rules = (rt_pair *)realloc(ps->rules, sizeof(rt_pair) * ps->_rules_cap * 2);
				ps->_rules_cap *= 2;
			}
			ps->rules[ps->num_rules].first = one_rule;
			ps->rules[ps->num_rules].second = ps->mitrees[i];
			rt_pair temp;
			for(int j = ps->num_rules - 1; j >= 0 && ps->rules[j].first->priority < ps->rules[j+1].first->priority; --j){
				temp = ps->rules[j];
				ps->rules[j] = ps->rules[j+1];
				ps->rules[j+1] = temp;
			}
			++ps->num_rules;
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
	rt_pair temp;
	for(int j = ps->num_rules - 1; j >= 0 && ps->rules[j].first->priority < ps->rules[j+1].first->priority; --j){
		temp = ps->rules[j];
		ps->rules[j] = ps->rules[j+1];
		ps->rules[j+1] = temp;
	}
	++ps->num_rules;

	if(ps->num_trees == ps->_tree_cap){
		ps->mitrees = (optimized_mi_tree **)realloc(ps->mitrees, sizeof(optimized_mi_tree *) * ps->_tree_cap * 2);
		ps->_tree_cap *= 2;
	}
	ps->mitrees[ps->num_trees++] = tree_ptr;  
	ps_insertion_sort_mitrees(ps);
}

void ps_delete_rule(struct partition_sort *ps, size_t i){
	if (i < 0 || i >= ps->num_rules) {
		printf("Warning index delete rule out of bound: do nothing here\n");
		printf("%lu vs. size: %u", i, ps->num_rules);
		return;
	}
	bool priority_change = false;

	struct optimized_mi_tree * mitree = ps->rules[i].second; 
	mit_deletion(mitree, ps->rules[i].first, &priority_change); 
 
	if (priority_change) {
		ps_insertion_sort_mitrees(ps);
	}


	if (mit_empty(mitree)) {
		ps->num_trees--;
		optimized_mi_tree_destroy(mitree);
	}


	if (i != ps->num_rules - 1) {
		ps->rules[i] = ps->rules[ps->num_rules - 1];
	}
	ps->num_rules--;


}

//Deletes a rule while maintaining order
//Assumes ps->rules is already in order
rule * ps_delete_rule_si(struct partition_sort *ps, size_t i);
rule * ps_delete_rule_si(struct partition_sort *ps, size_t i){

	if (i < 0 || i >= ps->num_rules) {
		printf("Warning index delete rule out of bound: do nothing here\n");
		printf("%lu vs. size: %u", i, ps->num_rules);
		return;
	}
	bool priority_change = false;

	rule *rule_to_delete = ps->rules[i].first;

	struct optimized_mi_tree * mitree = ps->rules[i].second; 
	mit_deletion(mitree, ps->rules[i].first, &priority_change); 
 
	if (priority_change) {
		ps_insertion_sort_mitrees(ps);
	}


	if (mit_empty(mitree)) {
		--ps->num_trees;
		optimized_mi_tree_destroy(mitree);
	}


	for(int j = i; j < ps->num_rules - 1; ++j){
		ps->rules[j] = ps->rules[j+1];
	}
	--ps->num_rules;
	
	return rule_to_delete;

}

//Deletes a rule while maintaining order
//Assumes ps->rules is already in order
rule * ps_delete_rule_s(struct partition_sort *ps, rt_pair *rtd){

	bool priority_change = false;

	rule *rule_to_delete = rtd->first;

	struct optimized_mi_tree * mitree = rtd->second; 
	mit_deletion(mitree, rtd->first, &priority_change); 
 
	if (priority_change) {
		ps_insertion_sort_mitrees(ps);
	}


	if (mit_empty(mitree)) {
		--ps->num_trees;
		optimized_mi_tree_destroy(mitree);
	}


	--ps->num_rules;
	*rtd = ps->rules[ps->num_rules];//Move last rule into spot occupied by deleted rule
	ps_insertion_sort_rules(ps);//One rule now out of order, insertion sort fastest
	
	return rule_to_delete;

}

//Taken from mbtree.c
struct match_counter {
  struct match match;
  int count;
};

//Taken from mbtree.c
static void
ps_count_match(struct match *match, struct match_list *match_counter_list) {
  struct match *match_counter;
  uint64_t length;

  TAILQ_FOREACH(match_counter, match_counter_list, entry) {
    if (match->oxm_field == match_counter->oxm_field) {
      length = match->oxm_length;
      if (OXM_FIELD_HAS_MASK(match->oxm_field)) {
        length >>= 1;
        /*if (memcmp(&match->oxm_value[length],
                   &match_counter->oxm_value[length],
                   length) != 0) {
          continue;
        }*/
      }
      ((struct match_counter *)match_counter)->count++;
      return;
    }
  }
  /* new match_counter add to the list */
  match_counter = calloc(1, sizeof(struct match_counter));
  memcpy(match_counter, match, sizeof(struct match));
  ((struct match_counter *)match_counter)->count = 1;
  TAILQ_INSERT_TAIL(match_counter_list, match_counter, entry);
}

//Taken from mbtree.c
static int
ps_count_flow_list_match(struct flow_list *flow_list,
                      struct match_list *match_counter_list) {
  struct flow *flow;
  struct match *match;
  int i, count;

  for (i = 0; i < flow_list->nflow; i++) {
    flow = flow_list->flows[i];
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_ETH_TYPE) {
        continue;
      }
      ps_count_match(match, match_counter_list);
    }
  }
  count = 0;
  TAILQ_FOREACH(match, match_counter_list, entry) {
    count++;
  }
  return count;
}

//Taken from mbtree.c
static int
ps_match_cmp(void *a, void *b) {
  struct match_counter *ma, *mb;

  ma = *(struct match_counter **)a;
  mb = *(struct match_counter **)b;

  return mb->count - ma->count;
}

//Taken from mbtree.c
static void
ps_get_match_counter_array(struct flow_list *flow_list, struct match_counter ***match_array, int *nmatch) {
  struct match_list match_counter_list;
  int i;

  TAILQ_INIT(&match_counter_list);
  *nmatch = ps_count_flow_list_match(flow_list, &match_counter_list);
  //printf("NUMBER OF MATCHES: %d\n", *nmatch);
  *match_array = calloc(*nmatch + 1, sizeof(struct match_counter *));
  for (i = 0; i < *nmatch; i++) {
    (*match_array)[i] = TAILQ_FIRST(&match_counter_list);
    TAILQ_REMOVE(&match_counter_list, TAILQ_FIRST(&match_counter_list), entry);
  }
  //qsort(*match_array, *nmatch, sizeof(struct match_counter *), ps_match_cmp);
}


//Chooses all of the fields used in the flows
//Arguments:
//	-fields is a null pointer which will end up pointing to the array of chosen fields
//	-num_fields is a null pointer which will end up pointing to the number of chosen fields 
void choose_classification_fields(struct flow_list *flow_list, uint8_t **fields, int *num_fields){
	int num_matches = 0;
	struct match_counter **match_array = NULL;
	ps_get_match_counter_array(flow_list, &match_array, &num_matches);

	//*num_fields = NUM_CLASSIFY_FIELDS > num_matches ? num_matches : NUM_CLASSIFY_FIELDS;
	*num_fields = num_matches;
	*fields = (uint8_t *)safe_malloc(sizeof(uint8_t) * (*num_fields));
	for(int i = 0; i < *num_fields; ++i){
		(*fields)[i] = match_array[i]->match.oxm_field;
	}
	free(match_array);
}

//Creates a classifier that classifies on the specified fields
//Arguments:
//	-flow_list, a pointer to the flow_list to be converted into a classifier
//	-fields, a pointer to the array of fields to be used
//	-NUM_FIELDS, the size of the array of fields
struct rule_vector* classifier_init(const struct flow_list *flow_list, uint8_t *fields, const int NUM_FIELDS){
	struct rule_vector *classifier = rule_vector_initc(flow_list->nflow);
	int nr = flow_list->nflow;
	struct rule *temp = NULL;

	for(int i = 0; i < nr; ++i){
		temp = rule_init_c(flow_list->flows[i], fields, NUM_FIELDS);
		//print_rule(temp);
		rule_vector_push_back(classifier, temp);//Stays in same order as flow_list 
	}

	return classifier;
}

//Replaces the classifier stored in flow_list with a new classifier based on the current state of flow_list->flows
void classifier_update_simple(struct flow_list *flow_list){
	struct rule_vector *new_classifier;
	uint8_t *fields; 
	int num_fields;

	choose_classification_fields(flow_list, &fields, &num_fields);
	new_classifier = classifier_init(flow_list, fields, num_fields);

	if(flow_list->classifier != NULL)
		classifier_destroy(flow_list->classifier);

	flow_list->classifier = new_classifier;
	ps_delete_classifier_l(flow_list->partition_sort);
	flow_list->partition_sort->construct_classifier(flow_list->partition_sort, new_classifier->items, new_classifier->size, fields, num_fields);

	free(fields);
}

void classifier_update_online(struct flow_list *flow_list){
	uint8_t *fields; 
	int num_fields;

	choose_classification_fields(flow_list, &fields, &num_fields);

	//printf("Field types: ");	
	//for(int i = 0; i < num_fields; ++i){
	//	printf("%u\t", fields[i] >> 1);
	//}
	//printf("\n");
	if(flow_list->partition_sort->_rules_cap != 0 && num_fields == flow_list->partition_sort->num_fields){//If classifier has been created and used the same number of fields
		bool same_fields = true;
		for(int i = 0; i < num_fields; ++i){
			bool has_field = false;
			for(int j = 0; j < num_fields; ++j){
				if(fields[j] == flow_list->partition_sort->fields[i])
					has_field = true;
			}
			if(!has_field){
				same_fields = false;
				break;
			}
		}
		if(same_fields){//If the classifier used the same fields
			//printf("Has same fields\n");
			//printf("Deleting rules\n");
			//Delete rules associated with discarded flows maintaining order
			//int num_to_delete = flow_list->_del_size;
			//for(int i = 0; i < num_to_delete; ++i){
			//	rule *rule_to_delete = ps_delete_rule_s(flow_list->partition_sort, flow_list->del_list[i]);
			//	rule_destroy(rule_to_delete);
			//}
	
			//printf("Adding rules\n");
			//Add all new flows to the classifier maintaining sorted order
			//rule *temp_rule = NULL;
			//for(int i = 0; i < rule_vector_size(flow_list->add_list); ++i){
			//	temp_rule = flow_list->add_list->items[i];
			//	ps_insert_rule_s(flow_list->partition_sort, temp_rule);s
			//}

			//flow_list->_del_size = 0;
			//rule_vector_clear(flow_list->add_list);
			//free(fields);

			return;//Classifier up to date
		}
	}

	//printf("Completely rebuilding classifier\n");
	//Need to completely rebuild classifier

	//flow_list->_del_size = 0;
	//for(int i = 0; i < rule_vector_size(flow_list->add_list); ++i){
	//	rule_destroy(flow_list->add_list->items[i]);
	//}
	//rule_vector_clear(flow_list->add_list);

	struct rule_vector *new_classifier;

	new_classifier = classifier_init(flow_list, fields, num_fields);

	if(flow_list->classifier != NULL)
		classifier_destroy(flow_list->classifier);

	flow_list->classifier = new_classifier;

	ps_delete_classifier_l(flow_list->partition_sort);

	flow_list->partition_sort->construct_classifier(flow_list->partition_sort, new_classifier->items, new_classifier->size, fields, num_fields);

	free(fields);
}


//Performs a binary search for the rule based on priority
//ASSUMES THE VECTOR IS SORTED DESCENDING BY PRIORITY
int ps_ifindp_b(partition_sort *ps, long long priority);
int ps_ifindp_b(partition_sort *ps, long long priority){
	
	int first = 0, last = ps->num_rules - 1;
	int mid = 0;
	int match_i = -1;

	while(first <= last){
	      mid = first + (last-first)/2;
	      if(ps->rules[mid].first->priority == priority){
		   match_i = mid;
		   break;
              }
	      else if(ps->rules[mid].first->priority > priority)
		 first = mid+1;
	      else
		 last = mid-1;
	}

	return match_i;
}

struct flow * ps_classify_an_l_packet(const struct lagopus_packet *pkt, struct flow_list *flow_list) {
	struct partition_sort *ps = flow_list->partition_sort;
	struct optimized_mi_tree *t = NULL;
	packet p = flow_list->pbuf;
	pbuf_init(pkt, ps->fields, ps->num_fields, p);
	struct flow *match = NULL;
	int result = -1;
	rule * temp_rule = NULL;

	//for(int i = 0; i < rule_vector_size(flow_list->classifier); ++i){
	//	print_rule(flow_list->classifier->items[i]);
	//}
	//printf("Packet: ");
	//for(int i = 0; i < ps->num_fields; i++){
	//	printf("%u\t", p[i]);
	//}
	//printf("\n");
	//exit(1);

	for (unsigned i = 0; i < ps->num_trees; ++i) {

		t = ps->mitrees[i];

		if (result > t->max_priority){
			break;
		}

		temp_rule = mit_classify_a_packet(t, p);
		if(temp_rule != NULL){ //&& rule_matches_l_packet(temp_rule, pkt)){//Packet has to match all of the fields, not just classification fields
			if(result == -1){
				result = temp_rule->master->priority;
				match = temp_rule->master;
			}
			else if(temp_rule->master->priority > match->priority){						
				result = temp_rule->master->priority;
				match = temp_rule->master;
			}
		}		
		
	}

	//printf("Partition sort result: %d\n", result);

	return match;
	
}

/*struct rt_pair * ps_find_rt_pair(struct flow_list *flow_list, int start_index){
	if(start_index < 0 || start_index > rule_vector_size(flow_list->classifier))
		return NULL;//Does not exist

	rule *rule_to_find = flow_list->classifier->items[start_index];//Classifier maintains insertions and deletions in real time, ps does not

	//If it is at the expected position in ps
	if(start_index < flow_list->partition_sort->num_rules && flow_list->partition_sort->rules[start_index].first == rule_to_find)
		return &flow_list->partition_sort->rules[start_index];

	//Need to search for it in ps
	int changed_rules = rule_vector_size(flow_list->add_list) + flow_list->_del_size;
	int lb = start_index - changed_rules >= 0 ? start_index - changed_rules : 0;
	int hb = start_index + changed_rules < flow_list->partition_sort->num_rules ? start_index + changed_rules : flow_list->partition_sort->num_rules - 1;

	int qsb = 0;
	int i = 0;		
	//Rule more likely to be closer to start_index than outer boundaries, so more efficient to check closer first
	if(start_index - lb > hb - start_index){
		qsb = hb - start_index;
		for(i = 1; i <= qsb; ++i){
			if(flow_list->partition_sort->rules[start_index - i].first == rule_to_find)
				return &flow_list->partition_sort->rules[start_index - i];
			if(flow_list->partition_sort->rules[start_index + i].first == rule_to_find)
				return &flow_list->partition_sort->rules[start_index + i];
		}
		while(start_index - i >= lb){
			if(flow_list->partition_sort->rules[start_index - i].first == rule_to_find)
				return &flow_list->partition_sort->rules[start_index - i];
			++i;	
		}	
	}
	else{
		qsb = start_index - lb;
		for(i = 1; i <= qsb; ++i){
			if(flow_list->partition_sort->rules[start_index - i].first == rule_to_find)
				return &flow_list->partition_sort->rules[start_index - i];
			if(flow_list->partition_sort->rules[start_index + i].first == rule_to_find)
				return &flow_list->partition_sort->rules[start_index + i];
		}
		while(i - start_index <= lb){
			if(flow_list->partition_sort->rules[start_index + i].first == rule_to_find)
				return &flow_list->partition_sort->rules[start_index + i];
			++i;	
		}
	}

	return NULL;//Did not find it within the boundary
		 
}*/
