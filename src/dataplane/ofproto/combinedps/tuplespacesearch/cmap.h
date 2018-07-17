/*
* Copyright (c) 2014 Nicira, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef CMAP_H
#define CMAP_H

#include <stdbool.h>
#include <stdint.h>
#include "hash.h"
#include "../utilities/ElementaryClasses.h"
#include "../utilities/int_vector.h"

/* Concurrent hash map
* ===================
*
* A single-writer, multiple-reader hash table that efficiently supports
* duplicates.
*
*
* Thread-safety
* =============
*
* The general rules are:
*
*    - Only a single thread may safely call into cmap_insert(),
*      cmap_remove(), or cmap_replace() at any given time.
*
*    - Any number of threads may use functions and macros that search or
*      iterate through a given cmap, even in parallel with other threads
*      calling cmap_insert(), cmap_remove(), or cmap_replace().
*
*      There is one exception: cmap_find_protected() is only safe if no thread
*      is currently calling cmap_insert(), cmap_remove(), or cmap_replace().
*      (Use ordinary cmap_find() if that is not guaranteed.)
*
*    - See "Iteration" below for additional thread safety rules.
*
* Writers must use special care to ensure that any elements that they remove
* do not get freed or reused until readers have finished with them.  This
* includes inserting the element back into its original cmap or a different
* one.  One correct way to do this is to free them from an RCU callback with
* ovsrcu_postpone().
*/

/* Given a pointer-typed lvalue OBJECT, expands to a pointer type that may be
* assigned to OBJECT. */
#ifdef __GNUC__
#define OVS_TYPEOF(OBJECT) typeof(OBJECT)
#else
#define OVS_TYPEOF(OBJECT) void *
#endif

/* Given OBJECT of type pointer-to-structure, expands to the offset of MEMBER
* within an instance of the structure.
*
* The GCC-specific version avoids the technicality of undefined behavior if
* OBJECT is null, invalid, or not yet initialized.  This makes some static
* checkers (like Coverity) happier.  But the non-GCC version does not actually
* dereference any pointer, so it would be surprising for it to cause any
* problems in practice.
*/
#ifdef __GNUC__
#define OBJECT_OFFSETOF(OBJECT, MEMBER) offsetof(typeof(*(OBJECT)), MEMBER)
#else
#define OBJECT_OFFSETOF(OBJECT, MEMBER) \
    ((char *) &(OBJECT)->MEMBER - (char *) (OBJECT))
#endif

/* Yields the size of MEMBER within STRUCT. */
#define MEMBER_SIZEOF(STRUCT, MEMBER) (sizeof(((STRUCT *) NULL)->MEMBER))

/* Given POINTER, the address of the given MEMBER in a STRUCT object, returns
the STRUCT object. */

#define CONTAINER_OF(POINTER, STRUCT, MEMBER)                           \
        ((STRUCT *) (void *) ((char *) (POINTER) - offsetof (STRUCT, MEMBER)))

/* Given POINTER, the address of the given MEMBER within an object of the type
* that that OBJECT points to, returns OBJECT as an assignment-compatible
* pointer type (either the correct pointer type or "void *").  OBJECT must be
* an lvalue.
*
* This is the same as CONTAINER_OF except that it infers the structure type
* from the type of '*OBJECT'. */
#define OBJECT_CONTAINING(POINTER, OBJECT, MEMBER)                      \
    ((OVS_TYPEOF(OBJECT)) (void *)                                      \
     ((char *) (POINTER) - OBJECT_OFFSETOF(OBJECT, MEMBER)))

/* Given POINTER, the address of the given MEMBER within an object of the type
* that that OBJECT points to, assigns the address of the outer object to
* OBJECT, which must be an lvalue.
*
* Evaluates to (void) 0 as the result is not to be used. */
#define ASSIGN_CONTAINER(OBJECT, POINTER, MEMBER) \
    ((OBJECT) = OBJECT_CONTAINING(POINTER, OBJECT, MEMBER), (void) 0)

/* As explained in the comment above OBJECT_OFFSETOF(), non-GNUC compilers
* like MSVC will complain about un-initialized variables if OBJECT
* hasn't already been initialized. To prevent such warnings, INIT_CONTAINER()
* can be used as a wrapper around ASSIGN_CONTAINER. */
#define INIT_CONTAINER(OBJECT, POINTER, MEMBER) \
    ((OBJECT) = NULL, ASSIGN_CONTAINER(OBJECT, POINTER, MEMBER))


/* A concurrent hash map node, to be embedded inside the data structure being
* mapped.
*
* All nodes linked together on a chain have exactly the same hash value. */
typedef struct cmap_node {
	unsigned int key;
	int priority;
	const rule *rule_ptr;//NEED TO MAKE TYPE OR CHANGE CODE
	struct cmap_node * next; /* Next node with same hash. */
} cmap_node;


static inline cmap_node * cmap_node_initk(unsigned int key){ 
    cmap_node * newNode = (cmap_node *)safe_malloc(sizeof(cmap_node));
    newNode->key = key;
    newNode->priority = -1;
    newNode->rule_ptr = NULL;
    newNode->next = NULL;
    return newNode;
}
static inline cmap_node * cmap_node_initr(const rule *r) { 
    cmap_node * newNode = (cmap_node *)safe_malloc(sizeof(cmap_node));
    newNode->key = -1;
    newNode->priority = r->priority;
    newNode->rule_ptr = r;
    newNode->next = NULL;
    return newNode;
}

static inline void cmap_node_free(cmap_node *node) {
    free(node);
}

static inline struct cmap_node *
cmap_node_next(const struct cmap_node *node)
{
	return node->next;
}

static inline struct cmap_node *
cmap_node_next_protected(const struct cmap_node *node)
{
	return node->next;
}

/* Concurrent hash map. */

typedef struct cmap {
	struct cmap_impl * impl;
} cmap;

/* Initialization. */
cmap * cmap_init();
void cmap_destroy(struct cmap *);

/* Count. */
size_t cmap_count(const struct cmap *);
bool cmap_is_empty(const struct cmap *);

/* Insertion and deletion.  Return the current count after the operation. */
size_t cmap_insert(struct cmap *, struct cmap_node *, uint32_t hash);
static inline size_t cmap_remove(struct cmap *, struct cmap_node *,
								 uint32_t hash);
size_t cmap_replace(struct cmap *, struct cmap_node *old_node,
struct cmap_node *new_node, uint32_t hash);

/* Search.
*
* These macros iterate NODE over all of the nodes in CMAP that have hash value
* equal to HASH.  MEMBER must be the name of the 'struct cmap_node' member
* within NODE.
*
* CMAP and HASH are evaluated only once.  NODE is evaluated many times.
*
*
* Thread-safety
* =============
*
* CMAP_NODE_FOR_EACH will reliably visit each of the nodes starting with
* CMAP_NODE, even with concurrent insertions and deletions.  (Of
* course, if nodes are being inserted or deleted, it might or might not visit
* the nodes actually being inserted or deleted.)
*
* CMAP_NODE_FOR_EACH_PROTECTED may only be used if the containing CMAP is
* guaranteed not to change during iteration.  It may be only slightly faster.
*
* CMAP_FOR_EACH_WITH_HASH will reliably visit each of the nodes with the
* specified hash in CMAP, even with concurrent insertions and deletions.  (Of
* course, if nodes with the given HASH are being inserted or deleted, it might
* or might not visit the nodes actually being inserted or deleted.)
*
* CMAP_FOR_EACH_WITH_HASH_PROTECTED may only be used if CMAP is guaranteed not
* to change during iteration.  It may be very slightly faster.
*/
#define CMAP_NODE_FOR_EACH(NODE, MEMBER, CMAP_NODE)                     \
    for (INIT_CONTAINER(NODE, CMAP_NODE, MEMBER);                       \
         (NODE) != OBJECT_CONTAINING(NULL, NODE, MEMBER);               \
         ASSIGN_CONTAINER(NODE, cmap_node_next(&(NODE)->MEMBER), MEMBER))
#define CMAP_NODE_FOR_EACH_PROTECTED(NODE, MEMBER, CMAP_NODE)           \
    for (INIT_CONTAINER(NODE, CMAP_NODE, MEMBER);                       \
         (NODE) != OBJECT_CONTAINING(NULL, NODE, MEMBER);               \
         ASSIGN_CONTAINER(NODE, cmap_node_next_protected(&(NODE)->MEMBER), \
                          MEMBER))
#define CMAP_FOR_EACH_WITH_HASH(NODE, MEMBER, HASH, CMAP)   \
    CMAP_NODE_FOR_EACH(NODE, MEMBER, cmap_find(CMAP, HASH))
#define CMAP_FOR_EACH_WITH_HASH_PROTECTED(NODE, MEMBER, HASH, CMAP)     \
    CMAP_NODE_FOR_EACH_PROTECTED(NODE, MEMBER, cmap_find_locked(CMAP, HASH))

struct cmap_node *cmap_find(const struct cmap *, uint32_t hash);
struct cmap_node *cmap_find_protected(const struct cmap *, uint32_t hash);

/* Looks up multiple 'hashes', when the corresponding bit in 'map' is 1,
* and sets the corresponding pointer in 'nodes', if the hash value was
* found from the 'cmap'.  In other cases the 'nodes' values are not changed,
* i.e., no NULL pointers are stored there.
* Returns a map where a bit is set to 1 if the corresponding 'nodes' pointer
* was stored, 0 otherwise.
* Generally, the caller wants to use CMAP_NODE_FOR_EACH to verify for
* hash collisions. */
unsigned long cmap_find_batch(const struct cmap *cmap, unsigned long map,
							  uint32_t hashes[],
							  const struct cmap_node *nodes[]);


struct cmap_cursor {
	const struct cmap_impl *impl;
	uint32_t bucket_idx;
	int entry_idx;
	struct cmap_node *node;
};

struct cmap_cursor cmap_cursor_start(const struct cmap *);
void cmap_cursor_advance(struct cmap_cursor *);


/* Iteration.
*
*
* Thread-safety
* =============
*
* Iteration is safe even in a cmap that is changing concurrently.  However:
*
*     - In the presence of concurrent calls to cmap_insert(), any given
*       iteration might skip some nodes and might visit some nodes more than
*       once.  If this is a problem, then the iterating code should lock the
*       data structure (a rwlock can be used to allow multiple threads to
*       iterate in parallel).
*
*     - Concurrent calls to cmap_remove() don't have the same problem.  (A
*       node being deleted may be visited once or not at all.  Other nodes
*       will be visited once.)
*
*
* Example
* =======
*
*     struct my_node {
*         struct cmap_node cmap_node;
*         int extra_data;
*     };
*
*     struct cmap_cursor cursor;
*     struct my_node *iter;
*     struct cmap my_map;
*
*     cmap_init(&cmap);
*     ...add data...
*     CMAP_FOR_EACH (my_node, cmap_node, &cursor, &cmap) {
*         ...operate on my_node...
*     }
*
* CMAP_FOR_EACH is "safe" in the sense of HMAP_FOR_EACH_SAFE.  That is, it is
* safe to free the current node before going on to the next iteration.  Most
* of the time, though, this doesn't matter for a cmap because node
* deallocation has to be postponed until the next grace period.  This means
* that this guarantee is useful only in deallocation code already executing at
* postponed time, when it is known that the RCU grace period has already
* expired.
*/

#define CMAP_CURSOR_FOR_EACH__(NODE, CURSOR, MEMBER)    \
    ((CURSOR)->node                                     \
     ? (INIT_CONTAINER(NODE, (CURSOR)->node, MEMBER),   \
        cmap_cursor_advance(CURSOR),                    \
        true)                                           \
     : false)

#define CMAP_CURSOR_FOR_EACH(NODE, MEMBER, CURSOR, CMAP)    \
    for (*(CURSOR) = cmap_cursor_start(CMAP);               \
         CMAP_CURSOR_FOR_EACH__(NODE, CURSOR, MEMBER);      \
        )

#define CMAP_CURSOR_FOR_EACH_CONTINUE(NODE, MEMBER, CURSOR)   \
	    while (CMAP_CURSOR_FOR_EACH__(NODE, CURSOR, MEMBER))

#define CMAP_FOR_EACH(NODE, MEMBER, CMAP)                       \
    for (struct cmap_cursor cursor__ = cmap_cursor_start(CMAP); \
         CMAP_CURSOR_FOR_EACH__(NODE, &cursor__, MEMBER);       \
        )

static inline struct cmap_node *cmap_first(const struct cmap *);

/* Another, less preferred, form of iteration, for use in situations where it
* is difficult to maintain a pointer to a cmap_node. */
struct cmap_position {
	unsigned int bucket;
	unsigned int entry;
	unsigned int offset;
};

struct cmap_node *cmap_next_position(const struct cmap *,
struct cmap_position *);

/* Returns the first node in 'cmap', in arbitrary order, or a null pointer if
* 'cmap' is empty. */
static inline struct cmap_node *
cmap_first(const struct cmap *cmap)
{
	struct cmap_position pos = { 0, 0, 0 };

	return cmap_next_position(cmap, &pos);
}

/* Removes 'node' from 'cmap'.  The caller must ensure that 'cmap' cannot
* change concurrently (from another thread).
*
* 'node' must not be destroyed or modified or inserted back into 'cmap' or
* into any other concurrent hash map while any other thread might be accessing
* it.  One correct way to do this is to free it from an RCU callback with
* ovsrcu_postpone().
*
* Returns the current number of nodes in the cmap after the removal. */
static inline size_t
cmap_remove(struct cmap *cmap, struct cmap_node *node, uint32_t hash)
{
	return cmap_replace(cmap, node, NULL, hash);
}

/*
* Calculates the size of the longest chain of nodes in the map which might have to be searched
* Added by James Daly
*/
int cmap_largest_chain(const struct cmap* cmap);

/*
* Finds the length of the array used to back this hash table
* Added by James Daly
*/
int cmap_array_size(const struct cmap* cmap);
#endif /* cmap.h */
