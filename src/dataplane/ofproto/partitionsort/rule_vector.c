#include "rule_vector.h"

rule_vector * rule_vector_init()
{
    rule_vector *v = (rule_vector *)safe_malloc(sizeof(rule_vector));
    v->capacity = R_DEFAULT_CAPACITY;
    v->size = 0;
    v->items = (rule **)safe_malloc(sizeof(rule *) * v->capacity);
    return v;
}

rule_vector * rule_vector_initc(const int CAPACITY)
{
    rule_vector *v = (rule_vector *)safe_malloc(sizeof(rule_vector));
    v->capacity = CAPACITY;
    v->size = 0;
    v->items = (rule **)safe_malloc(sizeof(rule *) * v->capacity);
    return v;
}

rule_vector * rule_vector_initr(rule **r, const int NUM_RULES)
{
    rule_vector *v = (rule_vector *)safe_malloc(sizeof(rule_vector));
    v->capacity = NUM_RULES;
    v->size = NUM_RULES;
    v->items = (rule **)safe_malloc(sizeof(rule *) * v->capacity);
    for(int i = 0; i < NUM_RULES; ++i){
	v->items[i] = r[i];
    }
    return v;
}


rule_vector* rule_vector_initv(const rule_vector *src)
{
    rule_vector *v = (rule_vector *)safe_malloc(sizeof(rule_vector));
    v->capacity = src->capacity;
    v->size = src->size;
    v->items = (rule **)safe_malloc(sizeof(rule *) * v->capacity);
    for(int i = 0; i < src->size; ++i){
	v->items[i] = src->items[i];
    }
    return v;
}

rule* rule_vector_get(rule_vector *v, int index)
{
    if (index >= 0 && index < v->size)
        return v->items[index];
    return NULL;
}

//Performs a binary search for the rule based on priority
//ASSUMES THE VECTOR IS SORTED DESCENDING BY PRIORITY
int rule_vector_ifindp_b(rule_vector *v, long long priority){
	int first = 0, last = v->size - 1;
	int mid = 0;
	int match_i = -1;

	while(first <= last){
	      mid = first + (last-first)/2;
	      if(v->items[mid]->priority == priority){
		   match_i = mid;
		   break;
              }
	      else if(v->items[mid]->priority > priority)
		 first = mid+1;
	      else
		 last = mid-1;
	}

	return match_i;
}


int rule_vector_find(rule_vector *v, long long p){
	for(int i = 0; i < v->size; i++){
		if(v->items[i]->priority == p){
			return i;
		}
	}
	return -1;
}

void rule_vector_iswap(rule_vector *v, int a, int b){
	rule *temp = v->items[a];
	v->items[a] = v->items[b];
	v->items[b] = temp;
}

static void rule_vector_resize(rule_vector *v, int capacity)
{
    rule **items = realloc(v->items, sizeof(rule *) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void rule_vector_push_back(rule_vector *v, rule *item)
{
    if (v->capacity == v->size)
        rule_vector_resize(v, v->capacity * 2);
    v->items[v->size] = item;
    ++v->size;
}

//Inserts the rule based on flow priority in descending order
//ASSUMES THE VECTOR WAS SORTED PRIOR TO INSERTION
int rule_vector_push_sort_f(rule_vector *v, rule *item)
{
    if (v->capacity == v->size)
        rule_vector_resize(v, v->capacity * 2);
    int count = v->size;
    v->items[v->size++] = item;
    rule *temp = v->items[count];
    while(count > 0 && v->items[count]->master->priority > v->items[count - 1]->master->priority){
	v->items[count] = v->items[count - 1];
	v->items[--count] = temp;
        temp = v->items[count];
    }
    return count;
}

void rule_vector_push_array(rule_vector *v, rule **ra, const int NUM_RULES){
	if(v->size + NUM_RULES > v->capacity){
        	rule_vector_resize(v, (v->size + NUM_RULES * 2));
	}
	int new_size = v->size + NUM_RULES;
	for(int i = v->size; i < new_size; ++i){
		v->items[i] = ra[i - v->size];
	}
	v->size += NUM_RULES;
}

void rule_vector_push_range(rule_vector *v1, rule_vector *v2){
	if(v1->size + v2->size > v1->capacity){
        	rule_vector_resize(v1, (v1->size + v2->size) * 2);
	}
	int new_size = v1->size + v2->size;
	for(int i = v1->size; i < new_size; ++i){
		v1->items[i] = v2->items[i - v1->size];
	}
	v1->size += v2->size;
}


void rule_vector_insert(rule_vector *v, int index, rule *r){
    	if (index < 0 || index >= v->size)
        	return;
	if (v->capacity == v->size)
	        rule_vector_resize(v, v->capacity * 2);
	memcpy(&v->items[index + 1], &v->items[index], sizeof(rule *) * (v->size - index));
	v->items[index] = r;
	v->size++;
}

void rule_vector_erase(rule_vector *v, int index)
{
    if (index < 0 || index >= v->size)
        return;

    v->items[index] = NULL;

    for (int i = index; i < v->size - 1; ++i) {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->size--;

}

void rule_vector_clear(rule_vector *v){
	v->size = 0;
	rule_vector_resize(v, R_DEFAULT_CAPACITY);
}

int rule_vector_compare_rule_p(void *l, void *r);
int rule_vector_compare_rule_p(void *l, void *r){
	if(((rule *)r)->priority - ((rule *)l)->priority){
		return -1;
	}
	else {
		return 1;
	}
}

//Sorts rules in vector in descending order based on priority.
//May want to change to use quicksort
void rule_vector_sort_p(rule_vector *v)
{
    	int cur_ind = 0;
    	rule *temp = NULL;
	
	for (int i = 1 ; i < v->size; i++) {
		cur_ind = i;
 
		while ( cur_ind > 0 && v->items[cur_ind]->priority > v->items[cur_ind-1]->priority) {
			temp = v->items[cur_ind];
			v->items[cur_ind] = v->items[cur_ind-1];
			v->items[cur_ind-1] = temp;
 
			--cur_ind;
		}
	}
	//qsort(v->items, v->size,  sizeof(rule *), rule_vector_compare_rule_p);
}

rule * rule_vector_max(rule_vector *v){
	if(v->size == 0)
		return NULL;
	rule *max = v->items[0];
	for(int i = 1; i < v->size; i++){
		if(v->items[i]->master->priority > max->master->priority)
			max = v->items[i];
	}
	return max;
}

