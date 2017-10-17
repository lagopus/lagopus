//Adaptation of Edd Mann's implementation of a simple vector
//http://eddmann.com/posts/implementing-a-dynamic-vector-array-in-c/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "misc.h"

#ifndef INT_VECTOR_H
#define INT_VECTOR_H

#define I_DEFAULT_CAPACITY 4

typedef struct int_vector {
    int *items;
    int capacity;
    int size;
} int_vector;

int_vector* int_vector_init(void);
int_vector* int_vector_initc(int capacity);
int_vector* int_vector_initv(int_vector *src);
int_vector* int_vector_inita(int *a, const int NUM_ITEMS);
void int_vector_pop(int_vector *v);
int int_vector_max(int_vector *v);
int int_vector_get(int_vector *v, int index);
int int_vector_find(int_vector *v, int item);
static void int_vector_resize(int_vector *, int);
void int_vector_push_back(int_vector *, int);
void int_vector_erase(int_vector *, int);
void int_vector_clear(int_vector *);

static inline void int_vector_free(int_vector *v)
{
	free(v->items);
	free(v);
}

static inline int * int_vector_to_array(int_vector *v){
	return v->items;
}

static inline void int_vector_set(int_vector *v, int index, int item)
{
    if (index >= 0 && index < v->size)
        v->items[index] = item;
}

static inline int int_vector_size(int_vector *v)
{
    return v->size;
}

#endif
