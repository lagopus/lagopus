//Adaptation of Edd Mann's implementation of a simple vector
//http://eddmann.com/posts/implementing-a-dynamic-vector-array-in-c/

#ifndef INT_VECTOR_H
#define INT_VECTOR_H

#include "misc.h"

#define I_DEFAULT_CAPACITY 4

typedef struct int_vector {
    int *items;
    int capacity;
    int size;
} int_vector;

int_vector* int_vector_init();
int_vector* int_vector_initc(int);
int_vector* int_vector_initv(int_vector *);
int_vector* int_vector_inita(int *, const int);
void int_vector_resize(int_vector *, int);
void int_vector_push_back(int_vector *, int);
int int_vector_max(int_vector *);
int int_vector_find(int_vector *, int);
void int_vector_push_range(int_vector *, int_vector *);

static inline void int_vector_free(int_vector *v){
    free(v->items);
    free(v);
}

static inline int int_vector_size(int_vector *v){ return v->size; }
static inline void int_vector_clear(int_vector *v){ v->size = 0; }
static inline int * int_vector_to_array(int_vector *v){ return v->items; }
static inline int int_vector_get(int_vector *v, int index){ return v->items[index]; }
static inline void int_vector_pop(int_vector *v){ --v->size; }
static inline void int_vector_set(int_vector *v, int index, int item) { v->items[index] = item; }
static inline void int_vector_erase(int_vector *v, int index) { memcpy(&(v->items[index]), &(v->items[index + 1]), sizeof(int) * (--v->size - index - 1)); }


#endif
