//Adaptation of Edd Mann's implementation of a simple vector
//http://eddmann.com/posts/implementing-a-dynamic-vector-array-in-c/

#include "ElementaryClasses.h"

#ifndef RULE_VECTOR_H
#define RULE_VECTOR_H
#define R_DEFAULT_CAPACITY 4

typedef struct rule_vector {
    rule **items;
    int capacity;
    int size;
} rule_vector;

rule_vector* rule_vector_init();
rule_vector* rule_vector_initr(rule**, const int);
rule_vector* rule_vector_initv(const rule_vector*);
void rule_vector_resize(rule_vector *, int);
void rule_vector_push_back(rule_vector *, rule *);
void rule_vector_push_range(rule_vector *, rule_vector *);
void rule_vector_iswap(rule_vector *, int, int);
int rule_vector_find(rule_vector *v, struct rule *r);

static inline void rule_vector_init_static(rule_vector *v){
	v->capacity = R_DEFAULT_CAPACITY;
	v->items = (struct rule **)calloc(v->capacity, sizeof(struct rule *));
}

static inline void rule_vector_free(rule_vector *v)
{
    free(v->items);
    free(v);
}

static inline void rule_vector_free_static(rule_vector *v){ free(v->items); }
static inline int rule_vector_size(rule_vector *v){ return v->size; }
static inline void rule_vector_pop(rule_vector *v){ v->size--; }
static inline void rule_vector_set(rule_vector *v, int index, rule *item){ v->items[index] = item; }
static inline rule* rule_vector_get(rule_vector *v, int index){ return v->items[index]; }
static inline void rule_vector_erase(rule_vector *v, int index){ memcpy(&(v->items[index]), &(v->items[index+1]), (--v->size - index) * sizeof(struct rule *)); }
static inline void rule_vector_clear(rule_vector *v){	v->size = 0; }
static inline rule ** rule_vector_to_array(rule_vector *v){ return v->items; }

#endif
