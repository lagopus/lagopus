#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "box_vector.h"
#include "misc.h"

box_vector * box_vector_init()
{
    box_vector *v = (box_vector *)safe_malloc(sizeof(box_vector));
    v->capacity = B_DEFAULT_CAPACITY;
    v->size = 0;
    v->items = (box *)safe_malloc(sizeof(box) * v->capacity);
    return v;
}

box_vector * box_vector_initv(box_vector *src)
{
    box_vector *v = (box_vector *)safe_malloc(sizeof(box_vector));
    v->capacity = src->capacity;
    v->size = src->size;
    v->items = (box *)safe_malloc(sizeof(box) * v->capacity);
    memcpy(v->items, src->items, sizeof(box) * src->size);
    return v;
}

void box_vector_resize(box_vector *v, int capacity)
{
    box *items = realloc(v->items, sizeof(box) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void box_vector_push_back(box_vector *v, box item)
{
    if (v->capacity == v->size)
        box_vector_resize(v, v->capacity * 2);
    v->items[v->size++] = item;
}

void box_vector_append(box_vector *dst, box_vector *src){
	if(dst->capacity <= dst->size + src->size)
		box_vector_resize(dst, dst->capacity + src->capacity);
	memcpy(&(dst->items[dst->size]), src->items, sizeof(box) * src->size);
	dst->size = dst->size + src->size;
}
