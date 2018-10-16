#ifndef GROUPER_H
#define GROUPER_H

#include <stdlib.h>

#include "../utilities/ElementaryClasses.h"
#include "../utilities/rule_vector.h"

typedef struct grouper{
	struct rule_vector *rules;
	int bits_per_group; //(b/t)
	int num_tables; //t
	int num_bytes; //b
	unsigned char **tables;
	unsigned char *classification_map;
} grouper;

struct grouper * grouper_init();
struct grouper * grouper_init_b(int num_bits_per_group);
void grouper_destroy(struct grouper *c);
void grouper_construct(struct grouper *c, rule_vector *rules);
void grouper_delete(struct grouper *c);
void grouper_rule_insert(struct grouper *c, struct rule *r, int i);
void grouper_rule_delete(struct grouper *c, struct rule *r, int i);
struct flow * grouper_classify_packet(struct grouper *c, packet p);
#endif
