#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rule_vector.h"

rule_vector * rule_vector_init()
{
    rule_vector *v = (rule_vector *)safe_malloc(sizeof(rule_vector));
    v->capacity = R_DEFAULT_CAPACITY;
    v->size = 0;
    v->items = (struct rule **)safe_malloc(sizeof(struct rule *) * v->capacity);
    return v;
}

rule_vector * rule_vector_initr(rule **r, const int NUM_RULES)
{
    rule_vector *v = (rule_vector *)safe_malloc(sizeof(rule_vector));
    v->capacity = NUM_RULES;
    v->size = NUM_RULES;
    v->items = (struct rule **)safe_malloc(sizeof(struct rule *) * v->capacity);

    memcpy(v->items, r, NUM_RULES * sizeof(struct rule *));
    return v;
}


rule_vector* rule_vector_initv(const rule_vector *src)
{
    rule_vector *v = (rule_vector *)safe_malloc(sizeof(rule_vector));
    v->capacity = src->capacity;
    v->size = src->size;
    v->items = (struct rule **)safe_malloc(sizeof(struct rule *) * v->capacity);

    memcpy(v->items, src->items, src->size * sizeof(struct rule *));
    return v;
}

void rule_vector_resize(rule_vector *v, int capacity)
{
    rule **items = realloc(v->items, sizeof(struct rule *) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void rule_vector_push_back(rule_vector *v, rule *item)
{
    if (v->capacity == v->size)
        rule_vector_resize(v, v->capacity * 2);
    v->items[v->size++] = item;
}

void rule_vector_push_range(rule_vector *v1, rule_vector *v2){
	if(v1->size + v2->size > v1->capacity){
        	rule_vector_resize(v1, (v1->size + v2->size) * 2);
	}
	memcpy(&(v1->items[v1->size]), v2->items, sizeof(struct rule *) * v2->size);	
        v1->size += v2->size;
}

void rule_vector_iswap(rule_vector *v, int a, int b){
	rule *temp = v->items[a];
	v->items[a] = v->items[b];
	v->items[b] = temp;
}

int rule_vector_find(rule_vector *v, struct rule *r){
	int i;
	for(i = 0; i < v->size; ++i){
		if(v->items[i] == r){
			return i;
		}
	}

	return -1;
}
