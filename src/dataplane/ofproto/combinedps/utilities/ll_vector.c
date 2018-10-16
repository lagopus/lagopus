#include "ll_vector.h"
#include <string.h>

ll_vector* ll_vector_init()
{
    ll_vector *v = (ll_vector *)safe_malloc(sizeof(ll_vector));
    v->capacity = L_DEFAULT_CAPACITY;
    v->size = 0;
    v->items = (long long *)safe_malloc(sizeof(long long) * v->capacity);
    return v;
}


ll_vector* ll_vector_initc(int capacity)
{
    ll_vector *v = (ll_vector *)safe_malloc(sizeof(ll_vector));
    v->capacity = capacity;
    v->size = 0;
    v->items = (long long *)safe_malloc(sizeof(long long) * v->capacity);
    return v;
}

ll_vector* ll_vector_initv(ll_vector *src)
{
    ll_vector *v = (ll_vector *)safe_malloc(sizeof(ll_vector));
    v->capacity = src->capacity;
    v->size = src->size;
    v->items = (long long *)safe_malloc(sizeof(long long) * v->capacity);
//    for(int i = 0; i < v->size; ++i){
//        v->items[i] = src->items[i];
//    }
    memcpy(v->items, src->items, sizeof(long long) * src->size);
    return v;
}


ll_vector* ll_vector_inita(long long *a, const int NUM_ITEMS){
    ll_vector *v = (ll_vector *)safe_malloc(sizeof(ll_vector));
    v->capacity = NUM_ITEMS;
    v->size = NUM_ITEMS;
    v->items = (long long *)safe_malloc(sizeof(long long) * v->capacity);
//    for(int i = 0; i < v->size; ++i){
//        v->items[i] = a[i];
//    }
    memcpy(v->items, a, sizeof(long long) * NUM_ITEMS);
    return v;
}

void ll_vector_pop(ll_vector *v){
	if(v->size > 0)
		v->size--;
}

long long ll_vector_max(ll_vector *v){
	if(v->size == 0)
		return INT_MIN;

	int index = 0;
	for(int i = 1; i < v->size; ++i){
		if(v->items[i] > v->items[index])
			index = i;
	}
	return v->items[index];	
}

long long ll_vector_get(ll_vector *v, int index)
{
    if (index >= 0 && index < v->size)
        return v->items[index];
    return 0;
}

long long ll_vector_find(ll_vector *v, long long item){
	for(int i = 0; i < v->size; ++i){
		if(v->items[i] == item)
			return i;
	}
	return -1;
}

void ll_vector_resize(ll_vector *v, int capacity)
{
    long long *items = realloc(v->items, sizeof(long long) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void ll_vector_push_back(ll_vector *v, long long item)
{
    if (v->capacity == v->size)
        ll_vector_resize(v, v->capacity * 2);
    v->items[v->size++] = item;
}

void ll_vector_erase(ll_vector *v, int index)
{
    if (index < 0 || index >= v->size)
        return;

//    for (int i = index; i < v->size - 1; ++i) {
//        v->items[i] = v->items[i + 1];
//        v->items[i + 1] = 0;
//    }

      
    memcpy(&(v->items[index]), &(v->items[index + 1]), sizeof(long long) * (v->size - index - 1));

    v->items[--v->size] = 0;

//    v->size--;

}

void ll_vector_clear(ll_vector *v){
	v->size = 0;
	ll_vector_resize(v, L_DEFAULT_CAPACITY);
}
