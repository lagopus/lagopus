#ifndef COMBINED_CLASSIFIER_H
#define COMBINED_CLASSIFIER_H

#include "../utilities/ElementaryClasses.h"
#include "../utilities/rule_vector.h"
#include "../partitionsort/PartitionSort.h"
#include "../generic/generic_classifier.h"

typedef struct combined_classifier {
	struct partition_sort *psort;
	struct generic_classifier *gp;
	struct rule_vector *contiguous_ruleset;
	struct rule_vector *discontiguous_ruleset;
	int num_fields;//Used by lagopus
	uint8_t *fields;//Used by lagopus
} combined_classifier;

struct combined_classifier * combined_classifier_init();
void combined_classifier_destroy(struct combined_classifier *cc);
void combined_classifier_rules_init(struct flow_list *flow_list);
void combined_classifier_rules_delete(struct combined_classifier *cc);
void combined_classifier_classifiers_construct_offline(struct combined_classifier *cc);
void combined_classifier_classifiers_construct_online(struct combined_classifier *cc);
void combined_classifier_classifiers_delete(struct combined_classifier *cc);
void combined_classifier_choose_fields(struct flow_list *flow_list); // Best classification fields
void combined_classifier_update_simple(struct flow_list *flow_list); // Simply destroys classifier in which it inserts and rebuilds
void combined_classifier_update_online(struct flow_list *flow_list); // Inserts into appropriate classifier, rebuilding if necessary
struct flow * combined_classifier_classify_l_packet(const struct lagopus_packet *pkt, struct flow_list *flow_list);
void combined_classifier_insert(struct combined_classifier *cc, struct rule *rule);
void combined_classifier_delete(struct combined_classifier *cc, struct rule *rule);
#endif
