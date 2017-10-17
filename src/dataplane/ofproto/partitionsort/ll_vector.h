//Adaptation of Edd Mann's implementation of a simple vector
//http://eddmann.com/posts/implementing-a-dynamic-vector-array-in-c/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "misc.h"

#ifndef LL_VECTOR_H
#define LL_VECTOR_H

#define L_DEFAULT_CAPACITY 4

typedef struct ll_vector {
    long long *items;
    int capacity;
    int size;
} ll_vector;

ll_vector* ll_vector_init(void);
ll_vector* ll_vector_initc(int capacity);
ll_vector* ll_vector_initv(ll_vector *src);
ll_vector* ll_vector_inita(long long *a, const int NUM_ITEMS);
void ll_vector_pop(ll_vector *v);
long long ll_vector_max(ll_vector *v);
long long ll_vector_get(ll_vector *v, int index);
long long ll_vector_find(ll_vector *v, long long item);
static void ll_vector_resize(ll_vector *, int);
void ll_vector_push_back(ll_vector *, long long);
void ll_vector_erase(ll_vector *, int);
void ll_vector_clear(ll_vector *);

static inline void ll_vector_free(ll_vector *v)
{
	free(v->items);
	free(v);
}

static inline long long * ll_vector_to_array(ll_vector *v){
	return v->items;
}

static inline void ll_vector_set(ll_vector *v, int index, long long item)
{
    if (index >= 0 && index < v->size)
        v->items[index] = item;
}

static inline int ll_vector_size(ll_vector *v)
{
    return v->size;
}

#endif
