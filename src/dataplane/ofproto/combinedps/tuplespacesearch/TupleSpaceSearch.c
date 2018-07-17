#include "TupleSpaceSearch.h"
#include "hash.h"

// Values for Bernstein Hash
#define HashBasis 5381
#define HashMult 33

static int tss_tuple_compare(const void *a, const void*b);

unsigned int tss_tuple_hash_rule(tss_tuple *tup, const rule *r) {
    uint32_t hash = 0;
    for (size_t i = 0; i < int_vector_size(tup->dims); i++) {
        hash = hash_add(hash, r->fields[int_vector_get(tup->dims,i)].range[0]);
    }
    return hash_finish(hash, 16);
}

unsigned int tss_tuple_hash_packet(tss_tuple *tup, const packet p){
    uint32_t hash = 0;
    uint32_t max_uint = 0xFFFFFFFF;

    for (size_t i = 0; i < int_vector_size(tup->dims); i++) {
        int len = int_vector_get(tup->lengths, i);
        uint32_t mask = len != 32 ? ~(max_uint >> len) : max_uint;
        hash = hash_add(hash, p[int_vector_get(tup->dims,i)] & mask);
    }
    return hash_finish(hash, 16);
}

struct flow * tss_tuple_find_match_packet(tss_tuple *tup, const packet p)  {


    cmap_node * found_node = cmap_find(tup->map_in_tuple, tss_tuple_hash_packet(tup,p));
    printf("Found node NULL? %s\n", found_node == NULL ? "YES" : "NO" );
    int priority = -1;
    struct flow *best_match = NULL;
    while (found_node != NULL) {
        if (discontiguous_rule_matches_packet(found_node->rule_ptr,p)) {
	    if(priority <= found_node->priority){
            	priority = found_node->priority;
		best_match = found_node->rule_ptr->master;
	    }
        }
        found_node = found_node->next;
    }
    return best_match;
}

void tss_tuple_deletion(tss_tuple *tup, const rule *r, bool *priority_change){
	//find node containing the rule
    unsigned int hash_r = tss_tuple_hash_rule(tup, r);
    cmap_node * found_node = cmap_find(tup->map_in_tuple, hash_r);
    while (found_node != NULL) {
        if (found_node->priority == r->priority) {
            cmap_remove(tup->map_in_tuple, found_node, hash_r);
            break;
            cmap_node_free(found_node);
        }
        found_node = found_node->next;
    }

}

void tss_tuple_insertion(tss_tuple *tup, const rule *r, bool *priority_change){

    if (r->priority > tup->max_priority) {
	tup->max_priority = r->priority;
	*priority_change = true;
    }
    u_multiset_insert(tup->priority_container, r->priority, 1);

    cmap_node * new_node = cmap_node_initr(r); /*key & rule*/
    cmap_insert(tup->map_in_tuple, new_node, tss_tuple_hash_rule(tup, r));

}

tuple_space_search * tss_init(){
    tuple_space_search *tss = (tuple_space_search *)safe_malloc(sizeof(tuple_space_search));
    tss->all_priority_tuples = umap_uint64_ptuple_init();
    tss->dims = int_vector_init();
    tss->rules = rule_vector_init();
    tss->priority_tuples_vector = ptuple_vector_init();
    return tss;
}

void tss_rule_insert(tuple_space_search *tss, rule *r) {
    bool priority_change = false;
    tss_tuple *hit = umap_uint64_ptuple_find(tss->all_priority_tuples, tss_key_rule_prefix(tss, r));
    if (hit != NULL) {
	//printf("Tuple exists\n");
        //there is a tuple
        tss_tuple_insertion(hit, r, &priority_change);
        if (priority_change) {
            tss_retain_vector_invariant(tss);
        }
    } else {
	//printf("Tuple does not exist\n");
        //create_tuple
        tss_tuple *ptuple = tss_tuple_init(tss->dims, r->prefix_length, r);
        umap_uint64_ptuple_insert(tss->all_priority_tuples, tss_key_rule_prefix(tss, r), ptuple);
        // add to priority vector
        ptuple_vector_push_back(tss->priority_tuples_vector, ptuple);
        tss_retain_vector_invariant(tss);
    }
    rule_vector_push_back(tss->rules, r);
}

void tss_rule_delete(tuple_space_search *tss, unsigned i) {

    if (i < 0 || i >= rule_vector_size(tss->rules)) {
//        printf("Warning index delete rule out of bound: do nothing here\n");
//        printf("%lu vs. size: %lu", i, rules.size());
        return;
    }
    
    bool priority_change = false;

    int key = tss_key_rule_prefix(tss, rule_vector_get(tss->rules, i));
    tss_tuple *hit = umap_uint64_ptuple_find(tss->all_priority_tuples, key);
    if (hit != NULL) {
        //there is a tuple
        tss_tuple_deletion(hit, rule_vector_get(tss->rules, i), &priority_change);
        if (tss_tuple_is_empty(hit)) {
            //destroy tuple and erase from the map
            umap_uint64_ptuple_erase(tss->all_priority_tuples, key);
            tss_tuple_destroy(hit);
            tss_retain_vector_invariant(tss);
            ptuple_vector_pop(tss->priority_tuples_vector);

        } else if (priority_change) {
            //sort tuple again
            tss_retain_vector_invariant(tss);
        }

    } else {
        //nothing to do?
        printf("Warning DeleteRule: no matching tuple in the rule; it should be here when inserted\n");
        exit(0);
    }
    if (i != rule_vector_size(tss->rules) - 1)
        rule_vector_set(tss->rules, i,  rule_vector_get(tss->rules, rule_vector_size(tss->rules) - 1));
    rule_vector_pop(tss->rules);
}

void tss_classifier_construct(tuple_space_search *tss, rule_vector *r){
    int i;

    if(r->size == 0) // Nothing to construct
	return;

//    printf("Preparing to push back dims\n");

    for(i = 0; i < r->items[0]->dim; ++i){ // COULD DETERMINE SMARTER ORDER TO DO THIS
//	printf("Iteration %i\n", i);
        int_vector_push_back(tss->dims, i);
    }

//    printf("Finished pushing back dims\n");

    for (i = 0; i < rule_vector_size(r); ++i) {
        tss_rule_insert(tss, rule_vector_get(r,i));
    }

//    printf("Number rules inserted: %d\n", rule_vector_size(tss->rules));
//    rule_vector_resize(tss->rules, 100000);
}

struct flow *tss_classify_packet(tuple_space_search *tss, const packet one_packet){
    //printf("Entering TSS_CLASSIFY_PACKET\n");
    int priority = -1;
    int num_tuples = ptuple_vector_size(tss->priority_tuples_vector);
    int i;	
    tss_tuple *current = NULL;
    struct flow *best_match = NULL, *result_flow;

    printf("Classifying packet\n");

    for(i = 0; i < num_tuples; ++i){
        current = ptuple_vector_get(tss->priority_tuples_vector, i);
        if (priority > current->max_priority) break;
        result_flow = tss_tuple_find_match_packet(current, one_packet);
	//printf("Found match\n");
	printf("%d, ", result_flow != NULL ? result_flow->priority : -1);
	if(result_flow != NULL && priority <= result_flow->priority){
            priority = result_flow->priority;
	    best_match = result_flow; 
	}       
    }
    printf("\n");
    exit(1);
    /*while (current = umap_uint64_ptuple_inext(tss->all_priority_tuples)) {
        //if (priority > current->max_priority) break;
        int result = tss_tupleFindMatchPacket(current, one_packet);
        priority = priority > result ? priority : result;
    }*/
    return best_match;
}

//MAX PRIORITY TUPLE BELONGS FIRST
static int tss_tuple_compare(const void *a, const void*b){
    const tss_tuple *lhs = (const tss_tuple *)(a);
    const tss_tuple *rhs = (const tss_tuple *)(b);
    if(lhs->max_priority < rhs->max_priority)
        return 1;
    else if(lhs->max_priority > rhs->max_priority)
        return -1;
    else
        return 0;
}

void tss_retain_vector_invariant(tuple_space_search *tss) {    
    /*static int count = 0;
    printf("\n");
    int i;
    for(i = 0; i < ptuple_vector_size(tss->priority_tuples_vector); ++i){
        printf("%d, ", ptuple_vector_get(tss->priority_tuples_vector, i)->max_priority);
    }
    printf("\n");*/
    int i,j;
    tss_tuple *temp = NULL;
    int ptuple_size = ptuple_vector_size(tss->priority_tuples_vector);
    //Should be partially sorted, use insertion sort
    for(j = 1; j < ptuple_size; ++j){
        for(i = j; i > 0; i--){
            if(tss->priority_tuples_vector->items[i]->max_priority > tss->priority_tuples_vector->items[i - 1]->max_priority){
                temp = tss->priority_tuples_vector->items[i];
                tss->priority_tuples_vector->items[i] = tss->priority_tuples_vector->items[i-1];
                tss->priority_tuples_vector->items[i-1] = temp;
            }
            else{
                break;
            }
        }
    }
    /*printf("\n");
    for(i = 0; i < ptuple_vector_size(tss->priority_tuples_vector); ++i){
        printf("%d, ", ptuple_vector_get(tss->priority_tuples_vector, i)->max_priority);
    }
    printf("\n");
    if(count == 10)
        exit(1);
    count++;*/
}
