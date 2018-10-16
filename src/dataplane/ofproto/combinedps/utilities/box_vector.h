//Adaptation of Edd Mann's implementation of a simple vector
//http://eddmann.com/posts/implementing-a-dynamic-vector-array-in-c/
#include "stdint.h"
#include "ElementaryClasses.h"

#ifndef BOX_VECTOR_H
#define BOX_VECTOR_H

#define B_DEFAULT_CAPACITY 4

#ifndef BOX_SIZE
#define BOX_SIZE 2
#endif

//typedef uint32_t Point;
typedef union rule_specifier box; //Box will always point to an array of BOX_SIZE points

typedef struct box_vector {
    box *items;
    int capacity;
    int size;
} box_vector;

box_vector * box_vector_init();
box_vector * box_vector_initv(box_vector *);
void box_vector_resize(box_vector *, int);
void box_vector_push_back(box_vector *, box);
void box_vector_append(box_vector *, box_vector *);

static inline void box_vector_free(box_vector *v)
{
    free(v->items);
    free(v);
}

static inline int box_vector_size(box_vector *v){ return v->size; }
static inline void box_vector_pop_back(box_vector *v){ --(v->size); }
static inline void box_vector_erase(box_vector *v, int index){ memcpy(&(v->items[index]), &(v->items[index + 1]), sizeof(box) * (--v->size - index)); }
static inline box box_vector_get(box_vector *v, int index){ return v->items[index]; }
static inline void box_vector_set(box_vector *v, int index, box item){ v->items[index] = item; }
static inline void box_vector_clear(box_vector *v){ v->size = 0; }
static inline box * box_vector_to_array(box_vector *v){	return v->items; }
#endif
