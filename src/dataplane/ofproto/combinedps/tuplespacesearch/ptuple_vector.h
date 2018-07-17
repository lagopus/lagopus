/* 
 * File:   ptuple_vector.h
 * Author: marcusj6
 *
 * Created on May 31, 2018, 12:04 PM
 */

#ifndef PTUPLE_VECTOR_H
#define	PTUPLE_VECTOR_H

#include "../utilities/misc.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define T_DEFAULT_CAPACITY 4

struct tss_tuple;
    
typedef struct ptuple_vector {
    struct tss_tuple **items;
    int capacity;
    int size;
} ptuple_vector;

ptuple_vector* ptuple_vector_init();
ptuple_vector* ptuple_vector_inita(struct tss_tuple**, const int);
ptuple_vector* ptuple_vector_initv(const ptuple_vector*);
int ptuple_vector_size(ptuple_vector *);
void ptuple_vector_resize(ptuple_vector *, int);
void ptuple_vector_push_back(ptuple_vector *, struct tss_tuple *);
void ptuple_vector_pop(ptuple_vector *);
void ptuple_vector_erase(ptuple_vector *, int);
void ptuple_vector_clear(ptuple_vector *);
void ptuple_vector_set(ptuple_vector *, int, struct tss_tuple *);
void ptuple_vector_push_range(ptuple_vector *, ptuple_vector *);
struct tss_tuple * ptuple_vector_get(ptuple_vector *, int);
void ptuple_vector_free(ptuple_vector *);
struct tss_tuple ** ptuple_vector_to_array(ptuple_vector *);
void ptuple_vector_iswap(ptuple_vector *, int, int);


#ifdef	__cplusplus
}
#endif

#endif	/* PTUPLE_VECTOR_H */

