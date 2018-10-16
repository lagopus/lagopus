#ifndef  TUPLE_H
#define  TUPLE_H

#include <stdlib.h>

#include "../utilities/misc.h"
#include "../utilities/int_vector.h"
#include "../utilities/rule_vector.h"
#include "../utilities/u_multiset.h"
#include "../utilities/ElementaryClasses.h"
#include "ptuple_vector.h"
#include "umap_uint64_ptuple.h"
#include "cmap.h"

#define POINTER_SIZE_BYTES 4

typedef struct tss_tuple{
    cmap *map_in_tuple;

    int_vector *dims;
    int_vector *lengths;
    int max_priority;
    u_multiset *priority_container;
} tss_tuple;

typedef struct tuple_space_search{
    rule_vector *rules;
    int_vector *dims;
    umap_uint64_ptuple *all_priority_tuples;
    ptuple_vector *priority_tuples_vector;
} tuple_space_search;

unsigned int tss_tuple_hash_rule(tss_tuple *tup, const rule *r);
unsigned int tss_tuple_hash_packet(tss_tuple *tup, const packet p);
struct flow * tss_tuple_find_match_packet(tss_tuple *tup, const packet p);
void tss_tuple_deletion(tss_tuple *tup, const rule *r, bool *priority_change);
void tss_tuple_insertion(tss_tuple *tup, const rule *r, bool *priority_change);
tuple_space_search * tss_init();
void tss_classifier_construct(tuple_space_search *tss, rule_vector *r);
struct flow * tss_classify_packet(tuple_space_search *tss, const packet one_packet);
void tss_rule_delete(tuple_space_search *tss, unsigned i);
void tss_rule_insert(tuple_space_search *tss, rule *one_rule);
void tss_retain_vector_invariant(tuple_space_search *tss);

static inline tss_tuple * tss_tuple_init(int_vector *dims, unsigned *lengths, const rule *r){
    tss_tuple *new_tuple = (tss_tuple *)safe_malloc(sizeof(tss_tuple));
    new_tuple->dims = int_vector_initv(dims);
    new_tuple->lengths = int_vector_inita(lengths, int_vector_size(dims));
    new_tuple->priority_container = u_multiset_create();
    new_tuple->max_priority = r->priority;
    u_multiset_insert(new_tuple->priority_container, r->priority, 1);
    new_tuple->map_in_tuple = cmap_init();
    bool unused = false;
    tss_tuple_insertion(new_tuple, r, &unused);
    return new_tuple;
}

static inline void tss_tuple_destroy(tss_tuple *tup) {
    cmap_destroy(tup->map_in_tuple);
    int_vector_free(tup->dims);
    int_vector_free(tup->lengths);
    u_multiset_destroy(tup->priority_container);
    free(tup);
    tup = NULL;
}

static inline int tss_tuple_count_rules(tss_tuple *tup) {
    return cmap_count(tup->map_in_tuple);
}

static inline bool tss_tuple_is_empty(tss_tuple *tup) { return tss_tuple_count_rules(tup) == 0; }

static inline void tss_delete(tuple_space_search *tss) {
    int i;
    umap_uint64_ptuple_clear(tss->all_priority_tuples);
    rule_vector_clear(tss->rules);
    int_vector_clear(tss->dims);
    for(i=ptuple_vector_size(tss->priority_tuples_vector)-1; i >= 0; --i){
        tss_tuple_destroy(ptuple_vector_get(tss->priority_tuples_vector, i));
        ptuple_vector_pop(tss->priority_tuples_vector);
    }
}

static inline void tss_destroy(tuple_space_search *tss) {
    umap_uint64_ptuple_free(tss->all_priority_tuples);
    if(tss->rules != NULL)
        rule_vector_free(tss->rules);
    int_vector_free(tss->dims);
    if(ptuple_vector_size(tss->priority_tuples_vector) !=0){
        int i;
        for(i=ptuple_vector_size(tss->priority_tuples_vector)-1; i >= 0; --i){
            tss_tuple_destroy(ptuple_vector_get(tss->priority_tuples_vector, i));
            ptuple_vector_pop(tss->priority_tuples_vector);
        }
    }
    ptuple_vector_free(tss->priority_tuples_vector);
    free(tss);
}

static inline int tss_get_num_tuples(tuple_space_search *tss){
    return ptuple_vector_size(tss->priority_tuples_vector);
}

static inline int tss_num_tables(tuple_space_search *tss) { return tss_get_num_tuples(tss); }

static inline int tss_rules_in_table(tuple_space_search *tss, int index) { return tss_tuple_count_rules(ptuple_vector_get(tss->priority_tuples_vector, index)); }

//John Regehr's left rotate (compiles to single machine instruction with GCC)
static inline uint32_t rotl32 (uint32_t x, unsigned int n)
{
  const unsigned int mask = (CHAR_BIT*sizeof(x)-1);

  assert ( (n<=mask) &&"rotate by type width or more");
  n &= mask;  // avoid undef behaviour with NDEBUG.  0 overhead for most types / compilers
  return (x<<n) | (x>>( (-n)&mask ));
}

static inline uint64_t tss_key_rule_prefix(tuple_space_search *tss, const rule *r) {
    int key = 0;
    int i;
    for (i = 0; i < int_vector_size(tss->dims); ++i) {
            //key <<= 6;
	    key = (int)rotl32(key, 6);
            key += (int)r->prefix_length[int_vector_get(tss->dims, i)];
    }
    return key;
}

#endif
