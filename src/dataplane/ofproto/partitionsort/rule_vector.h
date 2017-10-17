//Adaptation of Edd Mann's implementation of a simple vector
//http://eddmann.com/posts/implementing-a-dynamic-vector-array-in-c/

#include "ElementaryClasses.h"

#include <stdio.h>
#include <stdlib.h>

#include "misc.h"

#ifndef RULE_VECTOR_H
#define RULE_VECTOR_H
#define R_DEFAULT_CAPACITY 4

typedef struct rule_vector {
    rule **items;
    int capacity;
    int size;
} rule_vector;

rule_vector * rule_vector_init(void);
rule_vector * rule_vector_initc(const int );
rule_vector * rule_vector_initr(rule **, const int);
rule_vector* rule_vector_initv(const rule_vector *);
rule* rule_vector_get(rule_vector *, int);
int rule_vector_ifindp_b(rule_vector *, long long);
int rule_vector_find(rule_vector *, long long);
void rule_vector_iswap(rule_vector *, int, int);
static void rule_vector_resize(rule_vector *, int);
void rule_vector_push_back(rule_vector *, rule *);
int rule_vector_push_sort_f(rule_vector *, rule *);
void rule_vector_push_array(rule_vector *, rule **, const int);
void rule_vector_insert(rule_vector *, int, rule *);
void rule_vector_erase(rule_vector *, int);
void rule_vector_clear(rule_vector *);
void rule_vector_push_range(rule_vector *, rule_vector *);
void rule_vector_sort_p(rule_vector *);
rule * rule_vector_max(rule_vector *);

static inline rule ** rule_vector_to_array(rule_vector *v){
	return v->items;
}

static inline void rule_vector_free(rule_vector *v)
{
    free(v->items);
    free(v);
}

static inline int rule_vector_size(rule_vector *v)
{
    return v->size;
}


static inline void rule_vector_pop(rule_vector *v){
	if(v->size > 0)
		v->size--;
}

static inline void rule_vector_set(rule_vector *v, int index, rule *item)
{
    if (index >= 0 && index < v->size)
        v->items[index] = item;
}

#endif
