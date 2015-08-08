/*
 * From Tcl8.0.5/license.terms
 *
This software is copyrighted by the Regents of the University of
California, Sun Microsystems, Inc., Scriptics Corporation,
and other parties.  The following terms apply to all files associated
with the software unless explicitly disclaimed in individual files.

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
MODIFICATIONS.

GOVERNMENT USE: If you are acquiring this software on behalf of the
U.S. government, the Government shall have only "Restricted Rights"
in the software and related documentation as defined in the Federal
Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
are acquiring the software on behalf of the Department of Defense, the
software shall be classified as "Commercial Computer Software" and the
Government shall have only "Restricted Rights" as defined in Clause
252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
authors grant the U.S. Government and others acting in its behalf
permission to use and distribute the software in accordance with the
terms specified in this license.
*/

#ifndef __HASH_H__
#define __HASH_H__

typedef void *ClientData;

/*
 * Forward declaration of HashTable.  Needed by some C++ compilers
 * to prevent errors when the forward reference to HashTable is
 * encountered in the HashEntry structure.
 */

#ifdef __cplusplus
struct HashTable;
#endif

/*
 * Structure definition for an entry in a hash table.  No-one outside
 * ns should access any of these fields directly;  use the macros
 * defined below.
 */

typedef struct HashEntry {
  struct HashEntry *nextPtr;		/* Pointer to next entry in this
					 * hash bucket, or NULL for end of
					 * chain. */
  struct HashTable *tablePtr;		/* Pointer to table containing
                                         * entry. */
  struct HashEntry **bucketPtr;	/* Pointer to bucket that points to
					 * first entry in this entry's chain:
					 * used for deleting the entry. */
  ClientData clientData;		/* Application stores something here
					 * with SetHashValue. */
  union {				/* Key has one of these forms: */
    const void *oneWordKey;		/* One-word value for key. */
    char const string[0];		/* String for key.  The actual size
					 * will be as large as needed to hold
					 * the key. */
    uint8_t const bytes[0];		/* Multiple bytes for key.
					 * The actual size will be as large
					 * as necessary for this table's
					 * keys. */
  } key;				/* MUST BE LAST FIELD IN RECORD!! */
} HashEntry;

/*
 * Structure definition for a hash table.  Must be in ns.h so clients
 * can allocate space for these structures, but clients should never
 * access any fields in this structure.
 */

#define HASH_SMALL_HASH_TABLE 4
typedef struct HashTable {
  HashEntry **buckets;		/* Pointer to bucket array.  Each
					 * element points to first entry in
					 * bucket's hash chain, or NULL. */
  HashEntry *staticBuckets[HASH_SMALL_HASH_TABLE];
  /* Bucket array used for small tables
   * (to avoid mallocs and frees). */
  unsigned int numBuckets;		/* Total number of buckets allocated
					 * at **bucketPtr. */
  unsigned int numEntries;		/* Total number of entries present
					 * in table. */
  unsigned int rebuildSize;		/* Enlarge table when numEntries gets
					 * to be this large. */
  unsigned int downShift;		/* Shift count used in hashing
					 * function.  Designed to use high-
					 * order bits of randomized keys. */
  unsigned int mask;			/* Mask value used in hashing
					 * function. */
  unsigned int keyLen;		/* Type of keys used in this
					 * table.  It's either
					 * HASH_STRING_KEYS (zero)
					 * HASH_ONE_WORD_KEYS
					 * (sizeof(unsigned long int)),
					 * or an integer giving the
					 * number of ints that is the
					 * size (in bytes) of the key.
					 */
  unsigned int keyIntLen;		/* keyLen / SIZEOF_INT. */
  unsigned int keyModLen;		/* keyLen % SIZEOF_INT. */

  HashEntry *(*findProc) (struct HashTable *tablePtr,
                          const void *key);
  HashEntry *(*createProc) (struct HashTable *tablePtr,
                            const void *key, int *newPtr);
} HashTable;

/*
 * Structure definition for information used to keep track of searches
 * through hash tables:
 */

typedef struct HashSearch {
  HashTable *tablePtr;		/* Table being searched. */
  unsigned int nextIndex;		/* Index of next bucket to be
					 * enumerated after present one. */
  HashEntry *nextEntryPtr;	/* Next entry to be enumerated in the
					 * the current bucket. */
} HashSearch;

/*
 * Acceptable key types for hash tables:
 */

#define HASH_STRING_KEYS	LAGOPUS_HASHMAP_TYPE_STRING
#define HASH_ONE_WORD_KEYS	LAGOPUS_HASHMAP_TYPE_ONE_WORD

/*
 * Macros for clients to use to access fields of hash entries:
 */

#define GetHashValue(h) ((h)->clientData)
#define SetHashValue(h, value) ((h)->clientData = (ClientData) (value))
#define GetHashKey(tablePtr, h) \
  ((char *) (((tablePtr)->keyLen == HASH_ONE_WORD_KEYS) ? \
             (h)->key.oneWordKey : (h)->key.string))

/*
 * Macros to use for clients to use to invoke find and create procedures
 * for hash tables:
 */

#define FindHashEntry(tablePtr, key) \
  (*((tablePtr)->findProc))(tablePtr, key)
#define CreateHashEntry(tablePtr, key, newPtr) \
  (*((tablePtr)->createProc))(tablePtr, key, newPtr)

void		DeleteHashEntry(HashEntry *entryPtr);
void		DeleteHashTable(HashTable *tablePtr);
HashEntry 	*FirstHashEntry(HashTable *tablePtr,
                            HashSearch *searchPtr);
char 		*HashStats(HashTable *tablePtr);
void		InitHashTable(HashTable *tablePtr,
                      unsigned int keyType);
HashEntry 	*NextHashEntry (HashSearch *searchPtr);

#endif /* ! __HASH_H__ */
