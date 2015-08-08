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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hash.h"

/*
 * When there are this many entries per bucket, on average, rebuild
 * the hash table to make it larger.
 */

#define REBUILD_MULTIPLIER	3


/*
 * The following macro takes a preliminary integer hash value and
 * produces an index into a hash tables bucket list.  The idea is
 * to make it so that preliminary values that are arbitrarily similar
 * will end up in different buckets.  The hash function was taken
 * from a random-number generator.
 */

#if SIZEOF_VOID_P == SIZEOF_INT64_T
#define RANDOM_INDEX(tablePtr, i)                               \
  ((unsigned int)((((uint64_t)(i))*1103515245LL) >>           \
                  (tablePtr)->downShift) & (tablePtr)->mask)
#elif SIZEOF_VOID_P == SIZEOF_INT
#define RANDOM_INDEX(tablePtr, i)                               \
  ((unsigned int)((((unsigned int)(i))*1103515245LL) >>       \
                  (tablePtr)->downShift) & (tablePtr)->mask)
#else
#error Sorry we can not live like this.
#endif /* SIZEOF_VOID_P == SIZEOF_INT64_T ... */

/*
 * Procedure prototypes for static procedures in this file:
 */

static inline unsigned int	HashArray (HashTable *tablePtr,
                                       const void *key);
static HashEntry 	*ArrayFind (HashTable *tablePtr,
                              const void *key);
static HashEntry 	*ArrayCreate (HashTable *tablePtr,
                                const void *key, int *newPtr);
static HashEntry 	*BogusFind (HashTable *tablePtr,
                              const void *key);
static HashEntry 	*BogusCreate (HashTable *tablePtr,
                                const void *key, int *newPtr);
static inline unsigned int	HashString (const char *string);
static void		RebuildTable (HashTable *tablePtr);
static HashEntry 	*StringFind (HashTable *tablePtr,
                               const void *key);
static HashEntry 	*StringCreate (HashTable *tablePtr,
                                 const void *key, int *newPtr);
static HashEntry 	*OneWordFind (HashTable *tablePtr,
                                const void *key);
static HashEntry 	*OneWordCreate (HashTable *tablePtr,
                                  const void *key, int *newPtr);

/*
 *----------------------------------------------------------------------
 *
 * InitHashTable --
 *
 *	Given storage for a hash table, set up the fields to prepare
 *	the hash table for use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to FindHashEntry and
 *	CreateHashEntry.
 *
 *----------------------------------------------------------------------
 */

void
InitHashTable(HashTable *tablePtr, unsigned int keyLen) {
#if 0
  HashTable *tablePtr;	/* Pointer to table record, which is
                                 * supplied by the caller. */
  int keyLen;			/* Length of keys to use in table:
                                 * HASH_STRING_KEYS,
                                 * HASH_ONE_WORD_KEYS, or any other
                                 * integer (in bytes). */
#endif
  tablePtr->buckets = tablePtr->staticBuckets;
  tablePtr->staticBuckets[0] = tablePtr->staticBuckets[1] = 0;
  tablePtr->staticBuckets[2] = tablePtr->staticBuckets[3] = 0;
  tablePtr->numBuckets = HASH_SMALL_HASH_TABLE;
  tablePtr->numEntries = 0;
  tablePtr->rebuildSize = HASH_SMALL_HASH_TABLE*REBUILD_MULTIPLIER;
  tablePtr->downShift = 28;
  tablePtr->mask = 3;
  tablePtr->keyLen = keyLen;
  tablePtr->keyIntLen = 0;
  tablePtr->keyModLen = 0;
  if (keyLen == HASH_STRING_KEYS) {
    tablePtr->findProc = StringFind;
    tablePtr->createProc = StringCreate;
  } else if (keyLen <= HASH_ONE_WORD_KEYS) {
    tablePtr->keyLen = HASH_ONE_WORD_KEYS;
    tablePtr->findProc = OneWordFind;
    tablePtr->createProc = OneWordCreate;
  } else {
    div_t d = div((int)keyLen, (int)SIZEOF_INT);
    tablePtr->keyIntLen = (unsigned int)d.quot;
    tablePtr->keyModLen = (unsigned int)d.rem;
    tablePtr->findProc = ArrayFind;
    tablePtr->createProc = ArrayCreate;
  };
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteHashEntry --
 *
 *	Remove a single entry from a hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entry given by entryPtr is deleted from its table and
 *	should never again be used by the caller.  It is up to the
 *	caller to free the clientData field of the entry, if that
 *	is relevant.
 *
 *----------------------------------------------------------------------
 */

void
DeleteHashEntry(HashEntry *entryPtr) {
  HashEntry *prevPtr;

  if (*entryPtr->bucketPtr == entryPtr) {
    *entryPtr->bucketPtr = entryPtr->nextPtr;
  } else {
    for (prevPtr = *entryPtr->bucketPtr; ; prevPtr = prevPtr->nextPtr) {
      if (prevPtr == NULL) {
        fprintf(stderr, "malformed bucket chain in DeleteHashEntry.\n");
        abort();
      }
      if (prevPtr->nextPtr == entryPtr) {
        prevPtr->nextPtr = entryPtr->nextPtr;
        break;
      }
    }
  }
  entryPtr->tablePtr->numEntries--;
  free((char *) entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteHashTable --
 *
 *	Free up everything associated with a hash table except for
 *	the record for the table itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table is no longer useable.
 *
 *----------------------------------------------------------------------
 */

void
DeleteHashTable(HashTable *tablePtr) {
  HashEntry *hPtr, *nextPtr;
  size_t i;

  /*
   * Free up all the entries in the table.
   */

  for (i = 0; i < tablePtr->numBuckets; i++) {
    hPtr = tablePtr->buckets[i];
    while (hPtr != NULL) {
      nextPtr = hPtr->nextPtr;
      free((char *) hPtr);
      hPtr = nextPtr;
    }
  }

  /*
   * Free up the bucket array, if it was dynamically allocated.
   */

  if (tablePtr->buckets != tablePtr->staticBuckets) {
    free((char *) tablePtr->buckets);
  }

  /*
   * Arrange for panics if the table is used again without
   * re-initialization.
   */

  tablePtr->findProc = BogusFind;
  tablePtr->createProc = BogusCreate;
}

/*
 *----------------------------------------------------------------------
 *
 * FirstHashEntry --
 *
 *	Locate the first entry in a hash table and set up a record
 *	that can be used to step through all the remaining entries
 *	of the table.
 *
 * Results:
 *	The return value is a pointer to the first entry in tablePtr,
 *	or NULL if tablePtr has no entries in it.  The memory at
 *	*searchPtr is initialized so that subsequent calls to
 *	NextHashEntry will return all of the entries in the table,
 *	one at a time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HashEntry *
FirstHashEntry(HashTable *tablePtr, HashSearch *searchPtr) {
#if 0
  HashTable *tablePtr;		/* Table to search. */
  HashSearch *searchPtr;		/* Place to store information about
					 * progress through the table. */
#endif
  searchPtr->tablePtr = tablePtr;
  searchPtr->nextIndex = 0;
  searchPtr->nextEntryPtr = NULL;
  return NextHashEntry(searchPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * NextHashEntry --
 *
 *	Once a hash table enumeration has been initiated by calling
 *	FirstHashEntry, this procedure may be called to return
 *	successive elements of the table.
 *
 * Results:
 *	The return value is the next entry in the hash table being
 *	enumerated, or NULL if the end of the table is reached.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HashEntry *
NextHashEntry(HashSearch *searchPtr) {
#if 0
  HashSearch *searchPtr;	/* Place to store information about
                                 * progress through the table. Must
                                 * have been initialized by calling
                                 * FirstHashEntry. */
#endif
  HashEntry *hPtr;

  while (searchPtr->nextEntryPtr == NULL) {
    if (searchPtr->nextIndex >= searchPtr->tablePtr->numBuckets) {
      return NULL;
    }
    searchPtr->nextEntryPtr =
      searchPtr->tablePtr->buckets[searchPtr->nextIndex];
    searchPtr->nextIndex++;
  }
  hPtr = searchPtr->nextEntryPtr;
  searchPtr->nextEntryPtr = hPtr->nextPtr;
  return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * HashStats --
 *
 *	Return statistics describing the layout of the hash table
 *	in its hash buckets.
 *
 * Results:
 *	The return value is a malloc-ed string containing information
 *	about tablePtr.  It is the caller's responsibility to free
 *	this string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
HashStats(HashTable *tablePtr) {
#define NUM_COUNTERS 10
  int count[NUM_COUNTERS], overflow;
  size_t i, j;
  double average, tmp;
  HashEntry *hPtr;
  char *result = NULL;
  char *p;
  size_t resLen = (NUM_COUNTERS*60) + 300;

  /*
   * Compute a histogram of bucket usage.
   */

  for (i = 0; i < NUM_COUNTERS; i++) {
    count[i] = 0;
  }
  overflow = 0;
  average = 0.0;
  for (i = 0; i < tablePtr->numBuckets; i++) {
    j = 0;
    for (hPtr = tablePtr->buckets[i]; hPtr != NULL; hPtr = hPtr->nextPtr) {
      j++;
    }
    if (j < NUM_COUNTERS) {
      count[j]++;
    } else {
      overflow++;
    }
    tmp = (double)j;
    average += (tmp+1.0)*(tmp/tablePtr->numEntries)/2.0;
  }

  /*
   * Print out the histogram and a few other pieces of information.
   */

  result = (char *)malloc(resLen);
  if (result != NULL) {
    snprintf(result, resLen, "%d entries in table, %d buckets\n",
             tablePtr->numEntries, tablePtr->numBuckets);
    p = result + strlen(result);
    for (i = 0; i < NUM_COUNTERS; i++) {
      snprintf(p, resLen - (size_t)(p - result),
               "number of buckets with " PFSZ(u) " entries: %d\n",
               i, count[i]);
      p += strlen(p);
    }
    snprintf(p, resLen - (size_t)(p - result),
             "number of buckets with %d or more entries: %d\n",
             NUM_COUNTERS, overflow);
    p += strlen(p);
    snprintf(p, resLen - (size_t)(p - result),
             "average search distance for entry: %.1f", average);
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 *
 * HashString --
 *
 *	Compute a one-word summary of a text string, which can be
 *	used to generate a hash index.
 *
 * Results:
 *	The return value is a one-word summary of the information in
 *	string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static inline unsigned int
HashString(const char *s) {
  unsigned int ret = 0;

  /*
   * I tried a zillion different hash functions and asked many other
   * people for advice.  Many people had their own favorite functions,
   * all different, but no-one had much idea why they were good ones.
   * I chose the one below (multiply by 9 and add new character)
   * because of the following reasons:
   *
   * 1. Multiplying by 10 is perfect for keys that are decimal strings,
   *    and multiplying by 9 is just about as good.
   * 2. Times-9 is (shift-left-3) plus (old).  This means that each
   *    character's bits hang around in the low-order bits of the
   *    hash value for ever, plus they spread fairly rapidly up to
   *    the high-order bits to fill out the hash value.  This seems
   *    works well both for decimal and non-decimal strings.
   */

  while (*s != '\0') {
    ret += ((ret << 3) + (unsigned int)*s++);
  }
  return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * StringFind --
 *
 *	Given a hash table with string keys, and a string key, find
 *	the entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the
 *	hash table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static HashEntry *
StringFind(HashTable *tablePtr, const void *key) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find matching entry. */
#endif
  HashEntry *hPtr;
  unsigned int idx;

  idx = HashString(key) & tablePtr->mask;

  /*
   * Search all of the entries in the appropriate bucket.
   */

  for (hPtr = tablePtr->buckets[idx];
       hPtr != NULL;
       hPtr = hPtr->nextPtr) {
    if (strcmp((char *)key, hPtr->key.string) == 0) {
      return hPtr;
    }
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * StringCreate --
 *
 *	Given a hash table with string keys, and a string key, find
 *	the entry with a matching key.  If there is no matching entry,
 *	then create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this
 *	is a newly-created entry, then *newPtr will be set to a non-zero
 *	value;  otherwise *newPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */

static HashEntry *
StringCreate(HashTable *tablePtr, const void *vKey, int *newPtr) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find or create matching
				 * entry. */
  int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
#endif
  const char *key = (const char *)vKey;
  HashEntry *hPtr = NULL;

  if (key != NULL) {
    unsigned int idx = HashString(key) & tablePtr->mask;
    size_t kLen = strlen(key);

    /*
     * Search all of the entries in this bucket.
     */

    for (hPtr = tablePtr->buckets[idx];
         hPtr != NULL;
         hPtr = hPtr->nextPtr) {
      if (strcmp(key, hPtr->key.string) == 0) {
        *newPtr = 0;
        return hPtr;
      }
    }

    /*
     * Entry not found.  Add a new one to the bucket.
     */
    hPtr = (HashEntry *)malloc(sizeof(HashEntry) - sizeof(hPtr->key) +
                               kLen + 1);
    if (hPtr != NULL) {
      *newPtr = 1;
      hPtr->tablePtr = tablePtr;
      hPtr->bucketPtr = &(tablePtr->buckets[idx]);
      hPtr->nextPtr = *hPtr->bucketPtr;
      hPtr->clientData = 0;
      (void)memcpy((void *)(hPtr->key.string), (void *)key, kLen + 1);
      *hPtr->bucketPtr = hPtr;
      tablePtr->numEntries++;

      /*
       * If the table has exceeded a decent size, rebuild it with many
       * more buckets.
       */

      if (tablePtr->numEntries >= tablePtr->rebuildSize) {
        RebuildTable(tablePtr);
      }
    } else {
      *newPtr = 0;
    }
  } else {
    *newPtr = 0;
  }

  return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * OneWordFind --
 *
 *	Given a hash table with one-word keys, and a one-word key, find
 *	the entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the
 *	hash table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static HashEntry *
OneWordFind(HashTable *tablePtr, const void *key) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find matching entry. */
#endif
  HashEntry *hPtr;
  unsigned int idx;

  idx = RANDOM_INDEX(tablePtr, key);

  /*
   * Search all of the entries in the appropriate bucket.
   */

  for (hPtr = tablePtr->buckets[idx];
       hPtr != NULL;
       hPtr = hPtr->nextPtr) {
    if (hPtr->key.oneWordKey == key) {
      return hPtr;
    }
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * OneWordCreate --
 *
 *	Given a hash table with one-word keys, and a one-word key, find
 *	the entry with a matching key.  If there is no matching entry,
 *	then create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this
 *	is a newly-created entry, then *newPtr will be set to a non-zero
 *	value;  otherwise *newPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */

static HashEntry *
OneWordCreate(HashTable *tablePtr, const void *key, int *newPtr) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find or create matching
				 * entry. */
  int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
#endif
  HashEntry *hPtr;
  unsigned int idx;

  idx = RANDOM_INDEX(tablePtr, key);

  /*
   * Search all of the entries in this bucket.
   */

  for (hPtr = tablePtr->buckets[idx];
       hPtr != NULL;
       hPtr = hPtr->nextPtr) {
    if (hPtr->key.oneWordKey == key) {
      *newPtr = 0;
      return hPtr;
    }
  }

  /*
   * Entry not found.  Add a new one to the bucket.
   */

  hPtr = (HashEntry *)malloc(sizeof(HashEntry));
  if (hPtr != NULL) {
    *newPtr = 1;
    hPtr->tablePtr = tablePtr;
    hPtr->bucketPtr = &(tablePtr->buckets[idx]);
    hPtr->nextPtr = *hPtr->bucketPtr;
    hPtr->clientData = 0;
    hPtr->key.oneWordKey = key;
    *hPtr->bucketPtr = hPtr;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with
     * many more buckets.
     */

    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
      RebuildTable(tablePtr);
    }
  } else {
    *newPtr = 0;
  }
  return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * HashArray ==
 *
 *	Compute a one-word summary of an arbitrary byte array, which can be
 *	used to generate a hash index.
 *
 * Results:
 *	The return value is a one-word summary of the information in
 *	the given key.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static inline unsigned int
HashArray(HashTable *tablePtr, const void *key) {
  unsigned int ret = 0;
  unsigned int i;
  unsigned int *iPtr = (unsigned int *)key;

  if (tablePtr->keyModLen > 0) {
    char *p = (char *)key + tablePtr->keyIntLen * SIZEOF_INT;
    memcpy((void *)&ret, (void *)p, tablePtr->keyModLen);
  }

  for (i = 0; i < tablePtr->keyIntLen; i++) {
    ret += iPtr[i];
  }

  return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayFind --
 *
 *	Given a hash table with array-of-int keys, and a key, find
 *	the entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the
 *	hash table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static HashEntry *
ArrayFind(HashTable *tablePtr, const void *key) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find matching entry. */
#endif
  HashEntry *hPtr;
  unsigned int idx = HashArray(tablePtr, key);

  idx = RANDOM_INDEX(tablePtr, idx);

  /*
   * Search all of the entries in the appropriate bucket.
   */

  for (hPtr = tablePtr->buckets[idx];
       hPtr != NULL;
       hPtr = hPtr->nextPtr) {
    if (memcmp(key, (void *)hPtr->key.bytes, tablePtr->keyLen) == 0) {
      return hPtr;
    }
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayCreate --
 *
 *	Given a hash table with one-word keys, and a one-word key, find
 *	the entry with a matching key.  If there is no matching entry,
 *	then create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this
 *	is a newly-created entry, then *newPtr will be set to a non-zero
 *	value;  otherwise *newPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */

static HashEntry *
ArrayCreate(HashTable *tablePtr, const void *key, int *newPtr) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find or create matching
				 * entry. */
  int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
#endif
  HashEntry *hPtr = NULL;
  unsigned int idx = HashArray(tablePtr, key);

  idx = RANDOM_INDEX(tablePtr, idx);

  /*
   * Search all of the entries in the appropriate bucket.
   */

  for (hPtr = tablePtr->buckets[idx];
       hPtr != NULL;
       hPtr = hPtr->nextPtr) {
    if (memcmp(key, (void *)hPtr->key.bytes, tablePtr->keyLen) == 0) {
      *newPtr = 0;
      return hPtr;
    }
  }

  /*
   * Entry not found.  Add a new one to the bucket.
   */
  hPtr = (HashEntry *)malloc(sizeof(HashEntry) - sizeof(hPtr->key) +
                             tablePtr->keyLen);
  if (hPtr != NULL) {
    *newPtr = 1;
    hPtr->tablePtr = tablePtr;
    hPtr->bucketPtr = &(tablePtr->buckets[idx]);
    hPtr->nextPtr = *hPtr->bucketPtr;
    hPtr->clientData = 0;
    (void)memcpy((void *)hPtr->key.bytes, key, tablePtr->keyLen);
    *hPtr->bucketPtr = hPtr;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many
     * more buckets.
     */

    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
      RebuildTable(tablePtr);
    }
  } else {
    *newPtr = 0;
  }

  return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * BogusFind --
 *
 *	This procedure is invoked when an FindHashEntry is called
 *	on a table that has been deleted.
 *
 * Results:
 *	If panic returns (which it shouldn't) this procedure returns
 *	NULL.
 *
 * Side effects:
 *	Generates a panic.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static HashEntry *
BogusFind(HashTable *tablePtr, const void *key) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find matching entry. */
#endif
  (void)tablePtr;
  (void)key;
  fprintf(stderr, "called FindHashEntry on deleted table.\n");
  abort();
  return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * BogusCreate --
 *
 *	This procedure is invoked when an CreateHashEntry is called
 *	on a table that has been deleted.
 *
 * Results:
 *	If panic returns (which it shouldn't) this procedure returns
 *	NULL.
 *
 * Side effects:
 *	Generates a panic.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static HashEntry *
BogusCreate(HashTable *tablePtr, const void *key, int *newPtr) {
#if 0
  HashTable *tablePtr;	/* Table in which to lookup entry. */
  const void *key;		/* Key to use to find or create matching
				 * entry. */
  int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
#endif
  (void)tablePtr;
  (void)key;
  (void)newPtr;
  fprintf(stderr, "called CreateHashEntry on deleted table.\n");
  abort();
  return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *	This procedure is invoked when the ratio of entries to hash
 *	buckets becomes too large.  It creates a new table with a
 *	larger bucket array and moves all of the entries into the
 *	new table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get re-hashed to new
 *	buckets.
 *
 *----------------------------------------------------------------------
 */

static void
RebuildTable(HashTable *tablePtr) {
  unsigned int oldSize, count, idx;
  HashEntry **oldBuckets;
  HashEntry **oldChainPtr, **newChainPtr;
  HashEntry *hPtr;

  oldSize = tablePtr->numBuckets;
  oldBuckets = tablePtr->buckets;

  /*
   * Allocate and initialize the new bucket array, and set up
   * hashing constants for new array size.
   */

  tablePtr->numBuckets *= 4;
  tablePtr->buckets =
    (HashEntry **)malloc(tablePtr->numBuckets * sizeof(HashEntry *));
  for (count = tablePtr->numBuckets, newChainPtr = tablePtr->buckets;
       count > 0;
       count--, newChainPtr++) {
    *newChainPtr = NULL;
  }
  tablePtr->rebuildSize *= 4;
  tablePtr->downShift -= 2;
  tablePtr->mask = (tablePtr->mask << 2) + 3;

  /*
   * Rehash all of the existing entries into the new bucket array.
   */

  for (oldChainPtr = oldBuckets;
       oldSize > 0;
       oldSize--, oldChainPtr++) {
    for (hPtr = *oldChainPtr;
         hPtr != NULL;
         hPtr = *oldChainPtr) {
      *oldChainPtr = hPtr->nextPtr;
      if (tablePtr->keyLen == HASH_STRING_KEYS) {
        idx = HashString(hPtr->key.string) & tablePtr->mask;
      } else if (tablePtr->keyLen == HASH_ONE_WORD_KEYS) {
        idx = RANDOM_INDEX(tablePtr, hPtr->key.oneWordKey);
      } else {
        idx = HashArray(tablePtr, (const void *)hPtr->key.bytes);
        idx = RANDOM_INDEX(tablePtr, idx);
      }
      hPtr->bucketPtr = &(tablePtr->buckets[idx]);
      hPtr->nextPtr = *hPtr->bucketPtr;
      *hPtr->bucketPtr = hPtr;
    }
  }

  /*
   * Free up the old bucket array, if it was dynamically allocated.
   */

  if (oldBuckets != tablePtr->staticBuckets) {
    free((char *) oldBuckets);
  }
}
