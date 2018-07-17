#ifndef GENERIC_CLASSIFIER_H
#define GENERIC_CLASSIFIER_H

#include <stdlib.h>

#include "../utilities/ElementaryClasses.h"
#include "../utilities/rule_vector.h"

typedef struct generic_classifier{
	rule_vector *rules;
	#define GENERIC_CACHE_SIZE 10
	struct rule *rule_cache[GENERIC_CACHE_SIZE];
	#undef GENERIC_CACHE_SIZE
} generic_classifier;

struct generic_classifier * generic_classifier_init();
void generic_classifier_destroy(struct generic_classifier *c);
void generic_classifier_construct(struct generic_classifier *c, rule_vector *rules);
void generic_classifier_delete(struct generic_classifier *c);
void generic_classifier_rule_insert(struct generic_classifier *c, struct rule *r);
void generic_classifier_rule_delete(struct generic_classifier *c, struct rule *r);
struct flow * generic_classifier_classify_packet(struct generic_classifier *c, packet p, int current_best);
void generic_classifier_insertion_sort_rules(struct generic_classifier *c);
#endif
