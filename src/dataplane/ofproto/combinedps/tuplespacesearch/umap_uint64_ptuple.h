/* 
 * File:   umap_uint64_ptuple.h
 * Author: marcusj6
 *
 * Created on May 23, 2018, 1:24 PM
 */

#ifndef UMAP_UINT64_PTUPLE_H
#define	UMAP_UINT64_PTUPLE_H

#include "../utilities/misc.h"

#ifdef	__cplusplus
extern "C" {
#endif
    
struct tss_tuple;

typedef struct umap_uint64_ptuple umap_uint64_ptuple;
    
umap_uint64_ptuple * umap_uint64_ptuple_init();
int umap_uint64_ptuple_size(umap_uint64_ptuple *);
void umap_uint64_ptuple_free(umap_uint64_ptuple *);
void umap_uint64_ptuple_insert(umap_uint64_ptuple *, uint64_t, struct tss_tuple *);
void umap_uint64_ptuple_erase(umap_uint64_ptuple *, uint64_t);
struct tss_tuple * umap_uint64_ptuple_find(umap_uint64_ptuple *, uint64_t);
struct tss_tuple * umap_uint64_ptuple_inext(umap_uint64_ptuple *);
void umap_uint64_ptuple_ireset(umap_uint64_ptuple *);
void umap_uint64_ptuple_clear(umap_uint64_ptuple *);

#ifdef	__cplusplus
}
#endif

#endif	/* UMAP_UINT64_PTUPLE_H */

