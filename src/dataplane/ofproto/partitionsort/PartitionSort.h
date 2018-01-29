#ifndef  PSORT_H
#define  PSORT_H

#include "OptimizedMITree.h"
#include "SortableRulesetPartitioner.h"
#include "ElementaryClasses.h"
#include "rule_vector.h"
#include "int_vector.h"
#include "box_vector.h"

typedef struct rt_pair{
	struct rule *first;
	struct optimized_mi_tree *second;
} rt_pair;

typedef struct partition_sort {
	optimized_mi_tree **mitrees;
	unsigned num_trees;
	unsigned _tree_cap;
	struct rt_pair *rules;
	unsigned num_rules;
	unsigned _rules_cap;
	
	uint8_t *fields;//Used by lagopus
	int num_fields;//Used by lagopus

	void (*construct_classifier)(struct partition_sort *self, const rule **rules, const unsigned num_rules, const uint8_t *FIELDS, const int NUM_FIELDS);
} partition_sort;

void ps_delete_classifier_l(struct partition_sort *ps);
void ps_construct_classifier_online_l(struct partition_sort *self, const rule **rules, const unsigned num_rules, const uint8_t *FIELDS, const int NUM_FIELDS);
void ps_construct_classifier_offline_l(struct partition_sort *self, const rule **rules, const unsigned num_rules, const uint8_t *FIELDS, const int NUM_FIELDS);
void ps_delete_rule(struct partition_sort *ps, size_t index);
rule * ps_delete_rule_s(struct partition_sort *ps, rt_pair *rtd);
void ps_insert_rule(struct partition_sort *ps, const rule *one_rule);
void ps_insert_rule_s(struct partition_sort *ps, const rule *one_rule);
struct rule_vector* classifier_init(const struct flow_list *flow_list, uint8_t *fields, const int NUM_FIELDS); 
void choose_classification_fields(struct flow_list *flow_list, uint8_t **fields, int *num_fields);
void classifier_update_simple(struct flow_list *flow_list);
void classifier_update_online(struct flow_list *flow_list);
struct flow * ps_classify_an_l_packet(const struct lagopus_packet *pkt, struct flow_list *flow_list); 
//struct rt_pair * ps_find_rt_pair(struct flow_list *flow_list, int start_index);

static inline memory ps_mem_size_bytes(struct partition_sort *ps) {
	int size_total_bytes = 0;
	for (unsigned i = 0; i < ps->num_trees; ++i) {
		size_total_bytes += mit_memory_consumption(ps->mitrees[i]);
	}
	int size_array_pointers = ps->num_trees;
	int size_of_pointer = 4;
	return size_total_bytes + size_array_pointers*size_of_pointer;
}
static inline size_t ps_num_tables(struct partition_sort *ps) {
	return ps->num_trees;
}
static inline size_t ps_rules_in_table(struct partition_sort *ps, size_t index) { return ps->mitrees[index]->num_rules; }
static inline size_t ps_priority_of_table(struct partition_sort *ps, size_t index) { return ps->mitrees[index]->max_priority; }

static int compare_mitrees(void *a, void *b){
	optimized_mi_tree *m_a = (optimized_mi_tree *)(a), *m_b = (optimized_mi_tree *)(b);
	return m_b->max_priority - m_a->max_priority; 
}  

static inline void ps_insertion_sort_mitrees(struct partition_sort *ps) {
	int i, j, numLength = ps->num_trees;
	//qsort(ps->mitrees, numLength, sizeof(optimized_mi_tree *), compare_mitrees);
	struct optimized_mi_tree * key;
	for (j = 1; j < numLength; ++j)
	{
		key = ps->mitrees[j];
		for (i = j - 1; (i >= 0) && (ps->mitrees[i]->max_priority < key->max_priority); --i)
		{
			ps->mitrees[i + 1] = ps->mitrees[i];
		}
		ps->mitrees[i + 1] = key;
	}
}

static inline void ps_insertion_sort_rules(struct partition_sort *ps) {
	int i, j, numLength = ps->num_rules;
	struct rt_pair * key;
	for (j = 1; j < numLength; ++j)
	{
		key = &ps->rules[j];
		for (i = j - 1; (i >= 0) && (ps->rules[i].first->priority < key->first->priority); --i)
		{
			ps->rules[i + 1] = ps->rules[i];
		}
		ps->rules[i + 1] = *key;
	}
}

static inline partition_sort * create_partition_sort_online(){
	partition_sort *ps = (partition_sort *)safe_malloc(sizeof(partition_sort));
	ps->_tree_cap = 0;
	ps->_rules_cap = 0;
	ps->num_trees = 0;
	ps->num_rules = 0;
	ps->construct_classifier = ps_construct_classifier_online_l;
	return ps;
}

static inline partition_sort * create_partition_sort_offline(){
	partition_sort *ps = (partition_sort *)safe_malloc(sizeof(partition_sort));
	ps->_tree_cap = 0;
	ps->_rules_cap = 0;
	ps->num_trees = 0;
	ps->num_rules = 0;
	ps->construct_classifier = ps_construct_classifier_offline_l;
	return ps;
}

static inline int find_rule_with_flow_p(struct rule_vector *classifier, long long priority){
	return rule_vector_ifindp_b(classifier, priority);
}

//Destroys the classifier and all associated rules
static inline void classifier_destroy(struct rule_vector *classifier){
	for(int i = 0; i < classifier->size; ++i){
		rule_destroy(classifier->items[i]);
	}
	rule_vector_free(classifier);
}
#endif
