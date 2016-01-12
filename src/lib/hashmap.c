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





#include "hash.h"
#include "hash.c"





typedef struct lagopus_hashmap_record {
  lagopus_hashmap_type_t m_type;
  lagopus_rwlock_t m_lock;
  HashTable m_hashtable;
  lagopus_hashmap_value_freeup_proc_t m_del_proc;
  ssize_t m_n_entries;
  bool m_is_operational;
} lagopus_hashmap_record;





static inline void
s_read_lock(lagopus_hashmap_t hm, int *ostateptr) {
  if (hm != NULL && ostateptr != NULL) {
    (void)lagopus_rwlock_reader_enter_critical(&(hm->m_lock), ostateptr);
  }
}


static inline void
s_write_lock(lagopus_hashmap_t hm, int *ostateptr) {
  if (hm != NULL && ostateptr != NULL) {
    (void)lagopus_rwlock_writer_enter_critical(&(hm->m_lock), ostateptr);
  }
}


static inline void
s_unlock(lagopus_hashmap_t hm, int ostate) {
  if (hm != NULL) {
    (void)lagopus_rwlock_leave_critical(&(hm->m_lock), ostate);
  }
}


static inline bool
s_do_iterate(lagopus_hashmap_t hm,
             lagopus_hashmap_iteration_proc_t proc, void *arg) {
  bool ret = false;
  if (hm != NULL && proc != NULL) {
    HashSearch s;
    lagopus_hashentry_t he;

    for (he = FirstHashEntry(&(hm->m_hashtable), &s);
         he != NULL;
         he = NextHashEntry(&s)) {
      if ((ret = proc(GetHashKey(&(hm->m_hashtable), he),
                      GetHashValue(he),
                      he,
                      arg)) == false) {
        break;
      }
    }
  }
  return ret;
}


static inline lagopus_hashentry_t
s_find_entry(lagopus_hashmap_t hm, void *key) {
  lagopus_hashentry_t ret = NULL;

  if (hm != NULL) {
    ret = FindHashEntry(&(hm->m_hashtable), key);
  }

  return ret;
}


static inline lagopus_hashentry_t
s_create_entry(lagopus_hashmap_t hm, void *key) {
  lagopus_hashentry_t ret = NULL;

  if (hm != NULL) {
    int is_new;
    ret = CreateHashEntry(&(hm->m_hashtable), key, &is_new);
  }

  return ret;
}


static bool
s_freeup_proc(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  bool ret = false;
  (void)key;
  (void)he;

  if (arg != NULL) {
    lagopus_hashmap_t hm = (lagopus_hashmap_t)arg;
    if (hm->m_del_proc != NULL) {
      if (val != NULL) {
        hm->m_del_proc(val);
      }
      ret = true;
    }
  }

  return ret;
}


static inline void
s_freeup_all_values(lagopus_hashmap_t hm) {
  s_do_iterate(hm, s_freeup_proc, (void *)hm);
}


static inline void
s_clean(lagopus_hashmap_t hm, bool free_values) {
  if (free_values == true) {
    s_freeup_all_values(hm);
  }
  DeleteHashTable(&(hm->m_hashtable));
  (void)memset(&(hm->m_hashtable), 0, sizeof(HashTable));
  hm->m_n_entries = 0;
}


static inline void
s_reinit(lagopus_hashmap_t hm, bool free_values) {
  s_clean(hm, free_values);
  InitHashTable(&(hm->m_hashtable), (unsigned int)hm->m_type);
}





void
lagopus_hashmap_set_value(lagopus_hashentry_t he, void *val) {
  if (he != NULL) {
    SetHashValue(he, val);
  }
}


lagopus_result_t
lagopus_hashmap_create(lagopus_hashmap_t *retptr,
                       lagopus_hashmap_type_t t,
                       lagopus_hashmap_value_freeup_proc_t proc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_hashmap_t hm;

  if (retptr != NULL) {
    *retptr = NULL;
    hm = (lagopus_hashmap_t)malloc(sizeof(*hm));
    if (hm != NULL) {
      if ((ret = lagopus_rwlock_create(&(hm->m_lock))) ==
          LAGOPUS_RESULT_OK) {
        hm->m_type = t;
        InitHashTable(&(hm->m_hashtable), (unsigned int)t);
        hm->m_del_proc = proc;
        hm->m_n_entries = 0;
        hm->m_is_operational = true;
        *retptr = hm;
        ret = LAGOPUS_RESULT_OK;
      } else {
        free((void *)hm);
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_hashmap_shutdown(lagopus_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr, cstate);

  }
}


void
lagopus_hashmap_destroy(lagopus_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr, cstate);

    lagopus_rwlock_destroy(&((*hmptr)->m_lock));
    free((void *)*hmptr);
    *hmptr = NULL;
  }
}





static inline lagopus_result_t
s_clear(lagopus_hashmap_t *hmptr, bool free_values) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((*hmptr)->m_is_operational == true) {
    (*hmptr)->m_is_operational = false;
    s_reinit(*hmptr, free_values);
    (*hmptr)->m_is_operational = true;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_clear(lagopus_hashmap_t *hmptr, bool free_values) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_clear(hmptr, free_values);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_clear_no_lock(lagopus_hashmap_t *hmptr, bool free_values) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    ret = s_clear(hmptr, free_values);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_find(lagopus_hashmap_t *hmptr,
       void *key, void **valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_hashentry_t he;

  *valptr = NULL;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      *valptr = GetHashValue(he);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_find_no_lock(lagopus_hashmap_t *hmptr,
                             void *key, void **valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_find(hmptr, key, valptr);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_find(lagopus_hashmap_t *hmptr, void *key, void **valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {
    int cstate;

    s_read_lock(*hmptr, &cstate);
    {
      ret = s_find(hmptr, key, valptr);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_add(lagopus_hashmap_t *hmptr,
      void *key, void **valptr,
      bool allow_overwrite) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *oldval = NULL;
  lagopus_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      oldval = GetHashValue(he);
      if (allow_overwrite == true) {
        SetHashValue(he, *valptr);
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_ALREADY_EXISTS;
      }
    } else {
      he = s_create_entry(*hmptr, key);
      if (he != NULL) {
        SetHashValue(he, *valptr);
        (*hmptr)->m_n_entries++;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    }
    *valptr = oldval;
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_add(lagopus_hashmap_t *hmptr,
                    void *key, void **valptr,
                    bool allow_overwrite) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_add(hmptr, key, valptr, allow_overwrite);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_add_no_lock(lagopus_hashmap_t *hmptr,
                            void *key, void **valptr,
                            bool allow_overwrite) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_add(hmptr, key, valptr, allow_overwrite);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_delete(lagopus_hashmap_t *hmptr,
         void *key, void **valptr,
         bool free_value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = NULL;
  lagopus_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      val = GetHashValue(he);
      if (val != NULL &&
          free_value == true &&
          (*hmptr)->m_del_proc != NULL) {
        (*hmptr)->m_del_proc(val);
      }
      DeleteHashEntry(he);
      (*hmptr)->m_n_entries--;
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  if (valptr != NULL) {
    *valptr = val;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_delete(lagopus_hashmap_t *hmptr,
                       void *key, void **valptr,
                       bool free_value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_delete(hmptr, key, valptr, free_value);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_delete_no_lock(lagopus_hashmap_t *hmptr,
                               void *key, void **valptr,
                               bool free_value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    ret = s_delete(hmptr, key, valptr, free_value);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_iterate(lagopus_hashmap_t *hmptr,
          lagopus_hashmap_iteration_proc_t proc,
          void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((*hmptr)->m_is_operational == true) {
    if (s_do_iterate(*hmptr, proc, arg) == true) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_ITERATION_HALTED;
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_iterate(lagopus_hashmap_t *hmptr,
                        lagopus_hashmap_iteration_proc_t proc,
                        void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {
    int cstate;

    /*
     * The proc could modify hash values so we use write lock.
     */
    s_write_lock(*hmptr, &cstate);
    {
      ret = s_iterate(hmptr, proc, arg);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_iterate_no_lock(lagopus_hashmap_t *hmptr,
                                lagopus_hashmap_iteration_proc_t proc,
                                void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {

    ret = s_iterate(hmptr, proc, arg);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





lagopus_result_t
lagopus_hashmap_size(lagopus_hashmap_t *hmptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_read_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        ret = (*hmptr)->m_n_entries;
      } else {
        ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_size_no_lock(lagopus_hashmap_t *hmptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    if ((*hmptr)->m_is_operational == true) {
      ret = (*hmptr)->m_n_entries;
    } else {
      ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_hashmap_statistics(lagopus_hashmap_t *hmptr, const char **msgptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      msgptr != NULL) {
    int cstate;

    *msgptr = NULL;

    s_read_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        *msgptr = (const char *)HashStats(&((*hmptr)->m_hashtable));
        if (*msgptr != NULL) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }
      } else {
        ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
lagopus_hashmap_atfork_child(lagopus_hashmap_t *hmptr) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    lagopus_rwlock_reinitialize(&((*hmptr)->m_lock));
  }
}
