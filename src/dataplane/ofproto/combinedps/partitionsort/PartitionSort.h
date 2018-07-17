#ifndef  PSORT_H
#define  PSORT_H

#include "OptimizedMITree.h"
#include "SortableRulesetPartitioner.h"
#include "../utilities/ElementaryClasses.h"
#include "../utilities/rule_vector.h"
#include "../utilities/int_vector.h"
#include "../utilities/box_vector.h"

typedef struct rt_pair{
	struct rule *first;
	struct optimized_mi_tree *second;
} rt_pair;

typedef struct partition_sort {
	unsigned num_trees;
	unsigned _tree_cap;
	unsigned num_rules;
	unsigned _rules_cap;
	struct optimized_mi_tree **mitrees;
	struct rt_pair *rules;
	
} partition_sort;

void ps_classifier_delete(struct partition_sort *ps);
void ps_classifier_construct_online(struct partition_sort *self, struct rule **rules, const unsigned num_rules);
void ps_classifier_construct_offline(struct partition_sort *self, struct rule **rules, const unsigned num_rules);
void ps_rule_delete(struct partition_sort *ps, struct rule *one_rule);
void ps_rule_insert(struct partition_sort *ps, struct rule *one_rule);
struct flow * ps_classify_packet(struct partition_sort *ps, const packet p);
struct partition_sort * ps_init();
void ps_insertion_sort_mitrees(struct partition_sort *ps);
void ps_destroy(struct partition_sort *ps);

static inline size_t ps_num_tables(struct partition_sort *ps) {	return ps->num_trees; }
static inline size_t ps_rules_in_table(struct partition_sort *ps, size_t index) { return ps->mitrees[index]->num_rules; }
static inline size_t ps_priority_of_table(struct partition_sort *ps, size_t index) { return ps->mitrees[index]->max_priority; }

//static inline int compare_mitrees(void *a, void *b){
//	optimized_mi_tree *m_a = (optimized_mi_tree *)(a), *m_b = (optimized_mi_tree *)(b);
//	return m_b->max_priority - m_a->max_priority; 
//}  



#endif
