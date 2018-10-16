#include "grouper.h"

#define GROUPER_DEFAULT_BITS 8

struct grouper * grouper_init(){ 
	struct grouper *g = (struct grouper *)calloc(1, sizeof(struct grouper));
	g->bits_per_group = GROUPER_DEFAULT_BITS;
	return g;
}

struct grouper * grouper_init_b(int num_bits_per_group){ 
	struct grouper *g = (struct grouper *)calloc(1, sizeof(struct grouper));
	g->bits_per_group = num_bits_per_group;
	return g;
}

void grouper_destroy(struct grouper *g){
	if(g->tables){
		int i, j, max;
		for(i = 0, max = g->num_tables * (1 << g->bits_per_group); i < max; ++i){
			free(g->tables[i]);
		}
		free(g->tables);
	}
	if(g->classification_map)
		free(g->classification_map);
	free(g);
}

//ASSUMES RULES ARE SORTED DESCENDING BASED ON PRIORITY
void grouper_construct(struct grouper *g, rule_vector *rules){
	int i, j, max, num_rules, byte;
	int current_field, byte_in_field;
	struct rule *current;

	grouper_delete(g); // Make sure no memory already allocated

	g->rules = rules;
	num_rules = rule_vector_size(rules);

	g->classification_map = (unsigned char *)calloc((num_rules + 7)/ 8, sizeof(unsigned char)); // To be used with classification to avoid reallocing every time	

	if(num_rules != 0){		
		g->num_bytes = 4 * rule_vector_get(rules, 0)->dim;
		g->num_tables = (g->num_bytes * 8) / g->bits_per_group;
		g->tables = (unsigned char **)calloc(g->num_tables * (1 << g->bits_per_group), sizeof(unsigned char *));

		current_field = 0;
		byte_in_field = 0;

		for(i = 0, max = g->num_tables * (1 << g->bits_per_group); i < max; ++i){
			g->tables[i] = (unsigned char *)calloc((num_rules + 7)/ 8, sizeof(unsigned char));
			
			byte = i % g->bits_per_group;

			for(j = 0; j < num_rules; ++j){
				current = rule_vector_get(rules, j);
				if((current->fields[current_field].value & current->fields[current_field].mask) == byte){ // Matches this bitmap
					g->tables[i][j / 8] |= (1 << (j % 8));
				}
			}

			byte_in_field = (byte_in_field + 1) % 4;
			current_field = byte_in_field == 0 ? current_field + 1 : current_field;			
		}
	}	
}

void grouper_delete(struct grouper *g){
	if(g->tables){
		int i, max;
		for(i = 0, max = g->num_tables * g->bits_per_group; i < max; ++i){
			free(g->tables[i]);
		}
		free(g->tables);
		g->tables = NULL;
	}
	if(g->classification_map)
		free(g->classification_map);
	g->classification_map = NULL;
	g->num_bytes = 0;
	g->num_tables = 0;	
}

void grouper_rule_insert(struct grouper *g, struct rule *r, int index){
	//NOT SUPPORTED AT THE MOMENT
}

void grouper_rule_delete(struct grouper *g, struct rule *r, int index){
	//NOT SUPOORTED AT THE MOMENT
}

struct flow * grouper_classify_packet(struct grouper *g, packet p){
	int i, j, field, bytes_in_field, max;

	for(i = 0, max = (rule_vector_size(g->rules) + 7) / 8; i < max; ++i){ // Start with packet matching all rules
		g->classification_map[i] = 0xFF; 
	}

	field = 0;
	bytes_in_field = 0;

	for(i = 0; i < g->num_tables; ++i){
		for(j = 0, max = (rule_vector_size(g->rules) + 7)  / 8; j < max; ++j){
			g->classification_map[j] &= g->tables[i * ((p[field] >> (8 * bytes_in_field) & 0xFF))][j];
		}
		bytes_in_field = (bytes_in_field + 1) % 4;
		field = bytes_in_field == 0 ? field + 1 : field;
	}

	for(i = 0, max = (rule_vector_size(g->rules) + 7)  / 8; i < max; ++i){ // Find highest priority matching rule
		j = __builtin_ffs(g->classification_map[i]);
		if(j != 0){
			return rule_vector_get(g->rules, i * 8 + j)->master; 
		}
	}

	return NULL;
}

#undef GROUPER_DEFAULT_BITS 
