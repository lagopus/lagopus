#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "int_vector.h"

int_vector* int_vector_init()
{
    int_vector *v = (int_vector *)safe_malloc(sizeof(int_vector));
    v->capacity = I_DEFAULT_CAPACITY;
    v->size = 0;
    v->items = (int *)safe_malloc(sizeof(int) * v->capacity);
    return v;
}


int_vector* int_vector_initc(int capacity)
{
    int_vector *v = (int_vector *)safe_malloc(sizeof(int_vector));
    v->capacity = capacity;
    v->size = 0;
    v->items = (int *)safe_malloc(sizeof(int) * v->capacity);
    return v;
}

int_vector* int_vector_initv(int_vector *src)
{
    int_vector *v = (int_vector *)safe_malloc(sizeof(int_vector));
    v->capacity = src->capacity;
    v->size = src->size;
    v->items = (int *)safe_malloc(sizeof(int) * v->capacity);
    memcpy(v->items, src->items, sizeof(int) * src->size);
    return v;
}


int_vector* int_vector_inita(int *a, const int NUM_ITEMS){
    int_vector *v = (int_vector *)safe_malloc(sizeof(int_vector));
    v->capacity = NUM_ITEMS;
    v->size = NUM_ITEMS;
    v->items = (int *)safe_malloc(sizeof(int) * v->capacity);  
    memcpy(v->items, a, sizeof(int) * NUM_ITEMS);
    return v;
}

void int_vector_resize(int_vector *v, int capacity)
{
    int *items = realloc(v->items, sizeof(int) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void int_vector_push_back(int_vector *v, int item)
{
    if (v->capacity == v->size)
        int_vector_resize(v, v->capacity * 2);
    v->items[v->size++] = item;
}

int int_vector_max(int_vector *v){
	if(v->size == 0)
		return INT_MIN;

	int index = 0;
	for(int i = 1; i < v->size; i++){
		if(v->items[i] > v->items[index])
			index = i;
	}
	return v->items[index];	
}

int int_vector_find(int_vector *v, int item){
	for(int i = 0; i < v->size; i++){
		if(v->items[i] == item)
			return i;
	}
	return -1;
}

void int_vector_push_range(int_vector *v1, int_vector *v2){
	if(v1->size + v2->size > v1->capacity){
        	int_vector_resize(v1, (v1->size + v2->size) * 2);
	}
	memcpy(&(v1->items[v1->size]), v2->items, sizeof(int) * v2->size);	
        v1->size += v2->size;
}

