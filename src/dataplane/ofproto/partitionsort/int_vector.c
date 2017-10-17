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
    for(int i = 0; i < v->size; ++i){
        v->items[i] = src->items[i];
    }
    return v;
}


int_vector* int_vector_inita(int *a, const int NUM_ITEMS){
    int_vector *v = (int_vector *)safe_malloc(sizeof(int_vector));
    v->capacity = NUM_ITEMS;
    v->size = NUM_ITEMS;
    v->items = (int *)safe_malloc(sizeof(int) * v->capacity);
    for(int i = 0; i < v->size; ++i){
        v->items[i] = a[i];
    }
    return v;
}

void int_vector_pop(int_vector *v){
	if(v->size > 0)
		v->size--;
}

int int_vector_max(int_vector *v){
	if(v->size == 0)
		return INT_MIN;

	int index = 0;
	for(int i = 1; i < v->size; ++i){
		if(v->items[i] > v->items[index])
			index = i;
	}
	return v->items[index];	
}

int int_vector_get(int_vector *v, int index)
{
    if (index >= 0 && index < v->size)
        return v->items[index];
    return 0;
}

int int_vector_find(int_vector *v, int item){
	for(int i = 0; i < v->size; ++i){
		if(v->items[i] == item)
			return i;
	}
	return -1;
}

static void int_vector_resize(int_vector *v, int capacity)
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

void int_vector_erase(int_vector *v, int index)
{
    if (index < 0 || index >= v->size)
        return;

    for (int i = index; i < v->size - 1; ++i) {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = 0;
    }

    v->size--;

}

void int_vector_clear(int_vector *v){
	v->size = 0;
	int_vector_resize(v, I_DEFAULT_CAPACITY);
}
