#include <stdio.h>
#include <stdlib.h>

#include "ptuple_vector.h"

ptuple_vector * ptuple_vector_init()
{
    ptuple_vector *v = (ptuple_vector *)safe_malloc(sizeof(ptuple_vector));
    v->capacity = T_DEFAULT_CAPACITY;
    v->size = 0;
    v->items = (struct tss_tuple **)safe_malloc(sizeof(struct tss_tuple *) * v->capacity);
    return v;
}

ptuple_vector * ptuple_vector_inita(struct tss_tuple **r, const int NUM_TUPLES)
{
    ptuple_vector *v = (ptuple_vector *)safe_malloc(sizeof(ptuple_vector));
    v->capacity = NUM_TUPLES;
    v->size = NUM_TUPLES;
    v->items = (struct tss_tuple **)safe_malloc(sizeof(struct tss_tuple *) * v->capacity);
    for(int i = 0; i < NUM_TUPLES; i++){
	v->items[i] = r[i];
    }
    return v;
}


ptuple_vector* ptuple_vector_initv(const ptuple_vector *src)
{
    ptuple_vector *v = (ptuple_vector *)safe_malloc(sizeof(ptuple_vector));
    v->capacity = src->capacity;
    v->size = src->size;
    v->items = (struct tss_tuple **)safe_malloc(sizeof(struct tss_tuple *) * v->capacity);
    for(int i = 0; i < src->size; i++){
	v->items[i] = src->items[i];
    }
    return v;
}

int ptuple_vector_size(ptuple_vector *v)
{
    return v->size;
}

void ptuple_vector_resize(ptuple_vector *v, int capacity)
{
    struct tss_tuple **items = realloc(v->items, sizeof(struct tss_tuple *) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void ptuple_vector_push_back(ptuple_vector *v, struct tss_tuple *item)
{
    if (v->capacity == v->size)
        ptuple_vector_resize(v, v->capacity * 2);
    v->items[v->size++] = item;
}

void ptuple_vector_pop(ptuple_vector *v){
	if(v->size > 0)
		v->size--;
}

void ptuple_vector_set(ptuple_vector *v, int index, struct tss_tuple *item)
{
    if (index >= 0 && index < v->size)
        v->items[index] = item;
}


void ptuple_vector_push_range(ptuple_vector *v1, ptuple_vector *v2){
	if(v1->size + v2->size > v1->capacity){
        	ptuple_vector_resize(v1, (v1->size + v2->size) * 2);
	}
	int new_size = v1->size + v2->size;
	for(int i = v1->size; i < new_size; i++ ){
		v1->items[i] = v2->items[i - v1->size];
	}
	v1->size += v2->size;
}

struct tss_tuple* ptuple_vector_get(ptuple_vector *v, int index)
{
    if (index >= 0 && index < v->size)
        return v->items[index];
    return NULL;
}

void ptuple_vector_erase(ptuple_vector *v, int index)
{
    if (index < 0 || index >= v->size)
        return;

    v->items[index] = NULL;

    for (int i = index; i < v->size - 1; i++) {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->size--;

}

void ptuple_vector_clear(ptuple_vector *v){
	v->size = 0;
	ptuple_vector_resize(v, T_DEFAULT_CAPACITY);
}

void ptuple_vector_free(ptuple_vector *v)
{
    free(v->items);
    free(v);
}

struct tss_tuple ** ptuple_vector_to_array(ptuple_vector *v){
	return v->items;
}

void ptuple_vector_iswap(ptuple_vector *v, int a, int b){
	struct tss_tuple *temp = v->items[a];
	v->items[a] = v->items[b];
	v->items[b] = temp;
}

