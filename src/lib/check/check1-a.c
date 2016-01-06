/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lagopus_apis.h"


#


/*
 * A large key.
 */
typedef struct {
  uint64_t key;
  char name[256];
} aPair;





static inline aPair *
new_pair(uint64_t key, const char *name) {
  aPair *ret = (aPair *)malloc(sizeof(aPair));
  if (ret != NULL) {
    memset((void *)ret, 0, sizeof(aPair));
    ret->key = key;
    if (IS_VALID_STRING(name) == true) {
      snprintf(ret->name, sizeof(ret->name), "%s", name);
    }
  }
  return ret;
}


static void
delete_pair(void *p) {
  free(p);
}


static inline void
dump_pair(aPair *pPtr) {
  lagopus_msg_debug(1, "Key: " PF64S(20, u) ",\tval: '%s'\n",
                    pPtr->key,
                    (IS_VALID_STRING(pPtr->name) == true) ?
                    pPtr->name : "");
}


static bool
iter_proc(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  aPair *pPtr;
  bool ret = false;
  size_t *cPtr = (size_t *)arg;
  (void)key;
  (void)he;

  if (val != NULL) {
    pPtr = (aPair *)val;
    dump_pair(pPtr);
    if (cPtr != NULL) {
      *cPtr = *cPtr + 1;
    }
    ret = true;
  }

  if (ret == false) {
    lagopus_msg_error("?????\n");
  }
  return ret;
}


static inline void
llrand(uint64_t *vPtr) {
  uint64_t r0 = (uint64_t)random();
  uint64_t r1 = (uint64_t)random();
  *vPtr = (r0 << 32) | r1;
}





int
main(int argc, const char *const argv[]) {
  int ret = 1;
  aPair *pPtr;
  aPair *val;
  lagopus_result_t rc;
  lagopus_hashmap_t ht = NULL;
  size_t i;
  uint64_t key;
  size_t iter_count = 0;
  char valbuf[1024];
  size_t n_entry = 1000000;
  aPair *pairs = NULL;
  const char *msg = NULL;
  lagopus_chrono_t start, end;

  (void)argc;

  if (IS_VALID_STRING(argv[1]) == true) {
    int64_t tmp;
    if (lagopus_str_parse_int64(argv[1], &tmp) == LAGOPUS_RESULT_OK) {
      if (tmp > 0) {
        n_entry = (size_t)tmp;
      }
    }
  }
  pairs = (aPair *)malloc(sizeof(aPair) * n_entry);
  if (pairs == NULL) {
    goto done;
  }

  srand((unsigned int)time(NULL));

  /*
   * Creation
   */
  if ((rc = lagopus_hashmap_create(&ht,
                                   sizeof(aPair),
                                   delete_pair)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(rc);
    goto done;
  }

  /*
   * Insertion
   */
  for (i = 0; i < n_entry; i++) {
    llrand(&key);
    snprintf(valbuf, sizeof(valbuf), PF64(u), key);
    val = pPtr = new_pair(key, valbuf);
    rc = lagopus_hashmap_add(&ht, (void *)pPtr, (void **)&val, false);
    if (rc != LAGOPUS_RESULT_OK) {
      lagopus_perror(rc);
      goto done;
    } else {
      if (val != NULL) {
        lagopus_exit_fatal("val must be NULL.\n");
      }
    }
    pairs[i] = *pPtr;
  }

  /*
   * Iteration
   */
  start = lagopus_chrono_now();
  rc = lagopus_hashmap_iterate(&ht, iter_proc, &iter_count);
  if (rc != LAGOPUS_RESULT_OK) {
    lagopus_msg_error("iter_count: " PFSZS(20, u) "\n", iter_count);
    lagopus_perror(rc);
    goto done;
  }
  end = lagopus_chrono_now();
  fprintf(stdout,
          "Full iteration for " PFSZ(u) " entries:\t%15.3f usec.\n"
          "\t\t\t\t\t\t(%f nsec/iter)\n",
          n_entry, (double)(end - start) / 1000.0,
          (double)(end - start) / (double)n_entry);

  if (iter_count != n_entry) {
    lagopus_exit_fatal("# of entry and # of iteration mismatched.\n");
  }

  /*
   * Full match serch
   */
  start = lagopus_chrono_now();
  for (i = 0; i < n_entry; i++) {
    rc = lagopus_hashmap_find(&ht, (void *)&pairs[i], (void **)&pPtr);
    if (rc != LAGOPUS_RESULT_OK) {
      lagopus_perror(rc);
      lagopus_exit_fatal("rc must be LAGOPUS_RESULT_OK.\n");
    }
  }
  end = lagopus_chrono_now();
  fprintf(stdout,
          "Full-match search for " PFSZ(u) " entries:\t%15.3f usec.\n"
          "\t\t\t\t\t\t(%f nsec/search)\n",
          n_entry, (double)(end - start) / 1000.0,
          (double)(end - start) / (double)n_entry);

  /*
   * Random key search
   */
  for (i = 0; i < n_entry; i++) {
    llrand(&(pairs[i].key));
    snprintf(pairs[i].name, sizeof(pairs[i].name), PF64(u), pairs[i].key);
  }

  start = lagopus_chrono_now();
  for (i = 0; i < n_entry; i++) {
    rc = lagopus_hashmap_find(&ht, (void *)&pairs[i], (void **)&pPtr);
    if (rc != LAGOPUS_RESULT_OK && rc != LAGOPUS_RESULT_NOT_FOUND) {
      lagopus_perror(rc);
      lagopus_exit_fatal("rc must be LAGOPUS_RESULT_OK.\n");
    }
  }
  end = lagopus_chrono_now();
  fprintf(stdout,
          "Random key search for " PFSZ(u) " entries:\t%15.3f usec.\n"
          "\t\t\t\t\t\t(%f nsec/search)\n",
          n_entry, (double)(end - start) / 1000.0,
          (double)(end - start) / (double)n_entry);


  /*
   * Statistics
   */
  rc = lagopus_hashmap_statistics(&ht, &msg);
  if (rc != LAGOPUS_RESULT_OK) {
    lagopus_perror(rc);
  }
  fprintf(stdout, "%s\n", msg);
  free((void *)msg);

  ret = 0;

done:
  if (ht != NULL) {
    lagopus_hashmap_destroy(&ht, true);
  }
  free((void *)pairs);
  return ret;
}
