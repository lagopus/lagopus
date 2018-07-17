#include <time.h>

#include "CombinedClassifier.h"

struct combined_classifier * combined_classifier_init(){
	struct combined_classifier *cc = (struct combined_classifier *)malloc(sizeof(struct combined_classifier));
	cc->psort = ps_init();
	cc->gp = generic_classifier_init();
	cc->contiguous_ruleset = rule_vector_init();
	cc->discontiguous_ruleset = rule_vector_init();
	cc->num_fields = 0;
	cc->fields = NULL;

	return cc;
}

void combined_classifier_destroy(struct combined_classifier *cc){
	if(cc){
		ps_destroy(cc->psort);
		generic_classifier_destroy(cc->gp);
		combined_classifier_rules_delete(cc);
		rule_vector_free(cc->contiguous_ruleset);
		rule_vector_free(cc->discontiguous_ruleset);
		if(cc->fields)
			free(cc->fields);
		free(cc);
	}
}

void combined_classifier_rules_init(struct flow_list *flow_list){
	struct combined_classifier *cc = flow_list->cc;
	struct flow *current;
	int i;
	struct rule *r = NULL;
	for(i = 0; i < flow_list->nflow; ++i){
		current = flow_list->flows[i];
		r = rule_init_c(current, cc->fields, cc->num_fields);
		if(r->contiguous){
			rule_vector_push_back(cc->contiguous_ruleset, r);
//			printf("C:%d,",i);
		}
		else{
			rule_vector_push_back(cc->discontiguous_ruleset, r);
//			printf("D:%d,",i);
		}
	}
}

//NOTE - Invalidates cc->psort and cc->gp
void combined_classifier_rules_delete(struct combined_classifier *cc){
	int i;
	int size = rule_vector_size(cc->contiguous_ruleset);
	for(i = 0; i < size; ++i){ 
		rule_destroy(rule_vector_get(cc->contiguous_ruleset, i));
		rule_vector_set(cc->contiguous_ruleset, i, NULL);//Zeroing out memory to prevent errors (unecessary)
	}
	
	rule_vector_clear(cc->contiguous_ruleset);
	
	size = rule_vector_size(cc->discontiguous_ruleset);
	for(i = 0; i < size; ++i){ 
		rule_destroy(rule_vector_get(cc->discontiguous_ruleset, i));
		rule_vector_set(cc->discontiguous_ruleset, i, NULL);//Zeroing out memory to prevent errors (unecessary)
	}
	
	rule_vector_clear(cc->discontiguous_ruleset);
}

void combined_classifier_classifiers_construct_offline(struct combined_classifier *cc){
	ps_classifier_construct_offline(cc->psort, cc->contiguous_ruleset->items, cc->contiguous_ruleset->size);
	generic_classifier_construct(cc->gp, cc->discontiguous_ruleset);
}

void combined_classifier_classifiers_construct_online(struct combined_classifier *cc){
	ps_classifier_construct_online(cc->psort, cc->contiguous_ruleset->items, cc->contiguous_ruleset->size);
	generic_classifier_construct(cc->gp, cc->discontiguous_ruleset);
}

void combined_classifier_classifiers_delete(struct combined_classifier *cc){
	if(cc->psort)
		ps_classifier_delete(cc->psort);
	if(cc->gp)
		generic_classifier_delete(cc->gp);
}

//Start of code taken from mbtree.c
struct match_counter {
  struct match match;
  int count;
};

static void ps_count_match(struct match *match, struct match_list *match_counter_list);
static int ps_count_flow_list_match(struct flow_list *flow_list, struct match_list *match_counter_list);
static int ps_match_cmp(void *a, void *b);               
static void ps_get_match_counter_array(struct flow_list *flow_list, struct match_counter ***match_array, int *nmatch);

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

static int
ps_match_cmp(void *a, void *b) {
  struct match_counter *ma, *mb;

  ma = *(struct match_counter **)a;
  mb = *(struct match_counter **)b;

  return mb->count - ma->count;
}

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
//End of code take from mbtree.h

//Chooses all of the fields used in the flows
void combined_classifier_choose_fields(struct flow_list *flow_list){ // Best classification fields
	struct combined_classifier *cc = flow_list->cc;
	int num_matches = 0;
	struct match_counter **match_array = NULL;
	ps_get_match_counter_array(flow_list, &match_array, &num_matches);

//	*num_fields = NUM_CLASSIFY_FIELDS > num_matches ? num_matches : NUM_CLASSIFY_FIELDS;
	cc->num_fields = num_matches;
	cc->fields = (uint8_t *)safe_malloc(sizeof(uint8_t) * (cc->num_fields));
	for(int i = 0; i < cc->num_fields; ++i){
		cc->fields[i] = match_array[i]->match.oxm_field;
	}
	free(match_array);
}

void combined_classifier_update_simple(struct flow_list *flow_list){
	struct combined_classifier *cc;
	if(flow_list->cc == NULL){
		flow_list->cc = combined_classifier_init();
	}
	cc = flow_list->cc;
	combined_classifier_classifiers_delete(cc);
	combined_classifier_rules_delete(cc);
	if(cc->fields){
		free(cc->fields);		
	}	

	combined_classifier_choose_fields(flow_list);
	combined_classifier_rules_init(flow_list);
	combined_classifier_classifiers_construct_online(cc);
	
} // Simply destroys classifier in which it inserts\deletes and rebuilds

// Inserts into \ deletes from appropriate classifier, rebuilding if necessary
void combined_classifier_update_online(struct flow_list *flow_list){
	combined_classifier_update_simple(flow_list);
	//TODO - MAKE THIS ONLINE
} 

struct flow * combined_classifier_classify_l_packet(const struct lagopus_packet *pkt, struct flow_list *flow_list){
	packet p = flow_list->pbuf;
	if(p == NULL){
#define NUM_CLASSIFY_FIELDS 70
		flow_list->pbuf = pbuf_init(NUM_CLASSIFY_FIELDS);
#undef NUM_CLASSIFY_FIELDS
		p = flow_list->pbuf;
	}

	struct timespec begin, end;
	long elapsed;

	struct combined_classifier *cc = flow_list->cc;

//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin);
	pbuf_fill(pkt, cc->fields, cc->num_fields, p); // Make pbuf store extracted fields
//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
//	elapsed = end.tv_nsec - begin.tv_nsec;
//	printf("\nTime to create packet: %ld nanoseconds\n", elapsed);

//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin);
	struct flow *flow1 = ps_classify_packet(cc->psort, p);
//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
//	elapsed = end.tv_nsec - begin.tv_nsec;
//	printf("Time to run partitionsort: %ld nanoseconds\n", elapsed);

//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin);
	struct flow *flow2 = generic_classifier_classify_packet(cc->gp, p, flow1 == NULL ? -1 : flow1->priority);
//	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
//	elapsed = end.tv_nsec - begin.tv_nsec;
//	printf("Time to run generic classifier: %ld nanoseconds\n", elapsed);

//	exit(1);
	if(flow1 != NULL && flow2 != NULL){ // Flows found in both
		return flow1->priority > flow2->priority ? flow1 : flow2; 
	}
	else if(flow1 != NULL){ // Flow 2 is NULL
		return flow1;
	}
	else{ // Either flow1 is NULL or both are NULL
		return flow2;
	}
}

void combined_classifier_insert(struct combined_classifier *cc, struct rule *rule){
	//Decide whether to insert into PartitionSort or Tuple Space Search
	if(rule->contiguous){
		ps_rule_insert(cc->psort, rule);
	}
	else{
		generic_classifier_rule_insert(cc->gp, rule);		
	}
}

void combined_classifier_delete(struct combined_classifier *cc, struct rule *rule){
	//Decide whether to delete from PartitionSort or Tuple Space Search
	if(rule->contiguous){
		ps_rule_delete(cc->psort, rule);
	}
	else{
		generic_classifier_rule_delete(cc->gp, rule);		
	}
}

