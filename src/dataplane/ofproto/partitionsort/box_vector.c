#include "box_vector.h"

box_vector * box_vector_init()
{
    box_vector *v = (box_vector *)safe_malloc(sizeof(box_vector));
    v->capacity = B_DEFAULT_CAPACITY;
    v->size = 0;
    v->items = (box *)safe_malloc(sizeof(box) * v->capacity);
    return v;
}

box_vector * box_vector_initc(int capacity)
{
    box_vector *v = (box_vector *)safe_malloc(sizeof(box_vector));
    v->capacity = capacity;
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
    for(int i = 0; i < v->size; ++i){
	v->items[i] = (box)safe_malloc(sizeof(Point) * BOX_SIZE);
	v->items[i][0] = src->items[i][0];
	v->items[i][1] = src->items[i][1];
    }
    return v;
}

void box_vector_set(box_vector *v, int index, box item)
{
    if (index >= 0 && index < v->size)
        v->items[index][0] = item[0];
        v->items[index][1] = item[1];
}

box box_vector_get(box_vector *v, int index)
{
    if (index >= 0 && index < v->size)
        return v->items[index];
    return NULL;
}

static void box_vector_resize(box_vector *v, int capacity)
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
    v->items[v->size] = (box)safe_malloc(sizeof(Point) * BOX_SIZE);
    v->items[v->size][0] = item[0];
    v->items[v->size][1] = item[1];
    v->size++;
}

void box_vector_append(box_vector *dst, box_vector *src){
	if(dst->capacity <= dst->size + src->size)
		box_vector_resize(dst, dst->capacity + src->capacity);
	for(int i = dst->size; i < dst->size + src->size; ++i){
    		dst->items[i] = (box)safe_malloc(sizeof(Point) * BOX_SIZE);
		dst->items[i][0] = src->items[i - dst->size][0];
		dst->items[i][1] = src->items[i - dst->size][1];
	}
	dst->size = dst->size + src->size;
}

void box_vector_erase(box_vector *v, int index)
{
    if (index < 0 || index >= v->size)
        return;

    free(v->items[index]);

    for (int i = index; i < v->size - 1; ++i) {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->size--;

}

void box_vector_clear(box_vector *v){
	while(v->size > 0){
		free(v->items[--v->size]);
	}
	box_vector_resize(v, B_DEFAULT_CAPACITY);
}
