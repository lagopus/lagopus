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
#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "conv_json.h"





#define CTOR_IDX	LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 1

#define TOKEN_MAX	8192





typedef struct linereader_context {
  const char *m_filename;
  size_t m_lineno;
  lagopus_dstring_t m_buf;
  void *m_stream_in;
  void *m_stream_out;
  datastore_gets_proc_t m_gets_proc;
  datastore_printf_proc_t m_printf_proc;
  bool m_got_EOF;
} linereader_context;


typedef struct datastore_interp_record {
  lagopus_mutex_t m_lock;
  volatile const char *m_current_configurator;
  volatile datastore_interp_state_t m_status;
  volatile datastore_interp_state_t m_saved_status;
  linereader_context m_lr_ctx;
  bool m_is_allocd;
  bool m_is_stream;
  lagopus_hashmap_t m_cmd_tbl;
  lagopus_hashmap_t m_blocking_sessions;
  char m_atomic_auto_save_file[PATH_MAX];
  char *m_cmd_string;
} datastore_interp_record;


typedef struct objtbl_record {
  const char *m_name;
  lagopus_hashmap_t *m_hptr;
  datastore_update_proc_t m_upd_proc;
  datastore_enable_proc_t m_ebl_proc;
  datastore_serialize_proc_t m_srz_proc;
  datastore_destroy_proc_t m_dty_proc;
  datastore_compare_proc_t m_cmp_proc;
  datastore_getname_proc_t m_gnm_proc;
  datastore_duplicate_proc_t m_dup_proc;
} objtbl_record;
typedef objtbl_record *objtbl_t;


typedef struct configurator_record {
  const char *m_name;
  bool m_has_lock;
} configurator_record;
typedef configurator_record *configurator_t;


typedef struct obj_itr_ctx {
  void **m_objs;
  size_t m_n_objs;
  size_t m_idx;
} obj_itr_ctx;


typedef struct objlist_t {
  objtbl_t m_o;
  void **m_objs;
  size_t m_n_objs;
} objlist_t;


typedef struct session_info {
  struct session *session;
  lagopus_thread_t *thd;
} session_info_t;





static const char *const s_tbl_order[] = {
  "policer-action",
  "policer",
  "queue",
  "interface",
  "port",
  "channel",
  "controller",
  "bridge"
};
static const size_t s_n_tbl_order = sizeof(s_tbl_order) / sizeof(const char *);


static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static lagopus_hashmap_t s_obj_tbl;	/* The table on object tables. */
static lagopus_hashmap_t s_cnf_tbl;	/* The configurator table. */

static lagopus_mutex_t s_lock_lock;
static volatile configurator_t s_lock_holder;

static datastore_compare_proc_t s_cmp_proc = NULL;

static int
s_session_printf(void *stream, const char *fmt, ...)
__attr_format_printf__(2, 3);
static int
s_FILE_printf(void *stream, const char *fmt, ...)
__attr_format_printf__(2, 3);

static void	s_ctors(void) __attr_constructor__(CTOR_IDX);
static void	s_dtors(void) __attr_destructor__(CTOR_IDX);

static void	s_set_interp_state(datastore_interp_t ip,
                               datastore_interp_state_t s);
static datastore_interp_state_t	s_get_interp_state(datastore_interp_t ip);
static void	s_save_interp_state(datastore_interp_t ip);
static void	s_restore_interp_state(datastore_interp_t ip);





static void
s_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&s_obj_tbl);
  lagopus_hashmap_atfork_child(&s_cnf_tbl);
}


static void
s_objtbl_freeup(void *val) {
  if (val != NULL) {
    objtbl_t o = (objtbl_t)val;
    free((void *)(o->m_name));
    lagopus_hashmap_destroy(o->m_hptr, true);
    free(val);
  }
}


static void
s_configurator_freeup(void *val) {
  if (val != NULL) {
    configurator_t c = (configurator_t)val;
    free((void *)(c->m_name));
    free((void *)c);
  }
}

static lagopus_result_t
s_session_info_create(session_info_t **si,
                      struct session *session,
                      lagopus_thread_t *thd) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (si != NULL && session != NULL && thd != NULL) {
    *si = (session_info_t *) malloc(sizeof(struct session_info));
    if (*si != NULL) {
      (*si)->session = session;
      (*si)->thd = thd;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static void
s_session_info_destroy(session_info_t **si) {
  if (si != NULL) {
    free(*si);
    *si = NULL;
  }
}

static void
s_session_info_free(void *si) {
  s_session_info_destroy((session_info_t **) &si);
}


static void
s_once_proc(void) {
  lagopus_result_t r;

  if ((r = lagopus_mutex_create(&s_lock_lock)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize the configurator lock.\n");
  }

  if ((r = lagopus_hashmap_create(&s_obj_tbl, LAGOPUS_HASHMAP_TYPE_STRING,
                                  s_objtbl_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize an object schema table.\n");
  }

  if ((r = lagopus_hashmap_create(&s_cnf_tbl, LAGOPUS_HASHMAP_TYPE_STRING,
                                  s_configurator_freeup)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize an object schema table.\n");
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(10, "The datastore interpreter is initialized.\n");
}


static inline void
s_final(void) {
  lagopus_hashmap_destroy(&s_obj_tbl, true);
  lagopus_hashmap_destroy(&s_cnf_tbl, true);
}


static void
s_dtors(void) {
  s_final();

  lagopus_msg_debug(10, "The datastore interpreter is finalized.\n");
}





static inline void
s_unlink_atomic_auto_save_file(datastore_interp_t *iptr) {
  if (IS_VALID_STRING((*iptr)->m_atomic_auto_save_file) == true) {
    lagopus_msg_debug(1, "unlink auto save file: %s\n",
                      (*iptr)->m_atomic_auto_save_file);
    unlink((*iptr)->m_atomic_auto_save_file);
    (*iptr)->m_atomic_auto_save_file[0] = '\0';
  }
}

static inline int
s_is_writable_file(const char *file) {
  int ret = EINVAL;
  struct stat s;

  if (IS_VALID_STRING(file) == true) {
    errno = 0;
    if (stat(file, &s) == 0) {
      if (!(S_ISDIR(s.st_mode))) {
        ret = 0;
      } else {
        ret = EISDIR;
      }
    } else {
      if (errno == ENOENT) {
        ret = 0;
      }
    }
  } else {
    ret = EINVAL;
  }

  return ret;
}


static inline lagopus_result_t
s_add_tbl(const char *name,
          lagopus_hashmap_t *hptr,
          datastore_update_proc_t u_proc,
          datastore_enable_proc_t e_proc,
          datastore_serialize_proc_t s_proc,
          datastore_destroy_proc_t d_proc,
          datastore_compare_proc_t c_proc,
          datastore_getname_proc_t n_proc,
          datastore_duplicate_proc_t dup_proc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(name) == true &&
      hptr != NULL && *hptr != NULL &&
      u_proc != NULL &&
      e_proc != NULL &&
      s_proc != NULL &&
      d_proc != NULL &&
      c_proc != NULL &&
      n_proc != NULL &&
      dup_proc != NULL) {
    objtbl_t o = (objtbl_t)malloc(sizeof(*o));
    if (o != NULL) {
      const char *cpname = strdup(name);
      if (IS_VALID_STRING(cpname) == true) {
        void *val = (void *)o;
        o->m_name = cpname;
        o->m_hptr = hptr;
        o->m_upd_proc = u_proc;
        o->m_ebl_proc = e_proc;
        o->m_srz_proc = s_proc;
        o->m_dty_proc = d_proc;
        o->m_cmp_proc = c_proc;
        o->m_gnm_proc = n_proc;
        o->m_dup_proc = dup_proc;
        if ((ret = lagopus_hashmap_add(&s_obj_tbl, (void *)(o->m_name),
                                       &val, false)) != LAGOPUS_RESULT_OK) {
          free((void *)(o->m_name));
          free((void *)o);
        }
      } else {
        free((void *)cpname);
        free((void *)o);
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_delete_tbl(const char *name) {
  if (IS_VALID_STRING(name) == true) {
    (void)lagopus_hashmap_delete(&s_obj_tbl, (void *)name, NULL, true);
  }
}


static inline objtbl_t
s_find_tbl(const char *name) {
  objtbl_t ret = NULL;

  if (IS_VALID_STRING(name) == true) {
    void *val;
    if (lagopus_hashmap_find(&s_obj_tbl, (void *)name, &val) ==
        LAGOPUS_RESULT_OK) {
      ret = (objtbl_t)val;
    }
  }

  return ret;
}


static int
s_qcmp_proc(const void *v0, const void *v1, void *arg) {
  int ret = 0;

  (void) arg;

  if (s_cmp_proc != NULL) {
    /*
     * a dirty trick here.
     */
    const char **o0 = (const char **)v0;
    const char **o1 = (const char **)v1;
    int result = 0;
    lagopus_result_t r = s_cmp_proc((const void *)*o0,
                                    (const void *)*o1, &result);
    if (r == LAGOPUS_RESULT_OK) {
      ret = result;
    }
  }

  return ret;
}


static inline void
s_sort_objs(void **objs, size_t n_objs, datastore_compare_proc_t proc) {
  if (objs != NULL && n_objs > 0 && proc != NULL) {
    s_cmp_proc = proc;
    lagopus_qsort_r((void *)objs, n_objs, sizeof(void *), s_qcmp_proc, NULL);
    s_cmp_proc = NULL;
  }
}


static bool
s_list_objs_proc(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  bool ret = false;

  (void)key;
  (void)he;

  if (arg != NULL) {
    obj_itr_ctx *cptr = (obj_itr_ctx *)arg;
    if (cptr->m_idx < cptr->m_n_objs) {
      cptr->m_objs[cptr->m_idx++] = val;
      ret = true;
    }
  }

  return ret;
}


static inline lagopus_result_t
s_find_objs(const char *argv0, void ***listp, bool do_sort) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(argv0) == true) {
    objtbl_t o = s_find_tbl(argv0);

    if (o != NULL) {
      if (listp != NULL) {
        *listp = NULL;
      }

      if ((ret = lagopus_hashmap_size(o->m_hptr)) > 0) {
        if (listp != NULL) {
          size_t n_objs = (size_t)ret;

          if ((*listp = (void **)malloc(sizeof(void *) * n_objs)) != NULL) {
            obj_itr_ctx c;
            c.m_objs = *listp;
            c.m_n_objs = n_objs;
            c.m_idx = 0;

            if ((ret = lagopus_hashmap_iterate(o->m_hptr,
                                               s_list_objs_proc,
                                               (void *)&c)) ==
                LAGOPUS_RESULT_OK) {
              if (do_sort == true && o->m_cmp_proc != NULL) {
                s_sort_objs(*listp, n_objs, o->m_cmp_proc);
              }
              ret = (lagopus_result_t)n_objs;
            } else {
              free(*listp);
              *listp = NULL;
            }
          } else {
            ret = LAGOPUS_RESULT_NO_MEMORY;
          }
        }
      }
    } else {
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_destroy_all_objs_list(objlist_t *ol) {
  if (ol != NULL) {
    size_t i;

    for (i = 0; i < s_n_tbl_order; i++) {
      free((void *)ol[i].m_objs);
    }

    free((void *)ol);
  }
}


static inline lagopus_result_t
s_create_all_objs_list(objlist_t **objlistp) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (objlistp != NULL) {
    objlist_t *ol = NULL;

    *objlistp = NULL;
    if ((ol = (objlist_t *)malloc(sizeof(objlist_t) * s_n_tbl_order)) !=
        NULL) {
      size_t i;
      void **objs;
      objtbl_t o;

      for (i = 0; i < s_n_tbl_order; i++) {
        ol[i].m_objs = NULL;
        ol[i].m_n_objs = 0;
        o = ol[i].m_o = s_find_tbl(s_tbl_order[i]);
        if (o != NULL) {
          objs = NULL;
          if ((ret = s_find_objs(s_tbl_order[i], &objs, true)) > 0) {
            ol[i].m_objs = objs;
            ol[i].m_n_objs = (size_t)ret;
          } else if (ret < 0) {
            if (ret != LAGOPUS_RESULT_NOT_FOUND) {
              break;
            }
          }
        }
      }

      if (ret >= 0 || ret != LAGOPUS_RESULT_NOT_FOUND) {
        ret = (lagopus_result_t)s_n_tbl_order;
        *objlistp = ol;
      } else {
        s_destroy_all_objs_list(ol);
      }

    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_serialize(datastore_interp_t *iptr, lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && result != NULL) {
    objlist_t *ol = NULL;

    (void)lagopus_dstring_clear(result);

    /*
     * first of all, put all the non-object command (e.g. log, tls,
     * etc.) serialization very here. If it failed goto end.
     */
    (void)lagopus_dstring_appendf(result,
                                  "# all the log objects' attribute\n");
    if ((ret = log_cmd_serialize(result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }

    (void)lagopus_dstring_appendf(result,
                                  "# all the datastore objects' attribute\n");
    if ((ret = datastore_cmd_serialize(result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }

    (void)lagopus_dstring_appendf(result,
                                  "# all the agent objects' attribute\n");
    if ((ret = agent_cmd_serialize(result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }

    (void)lagopus_dstring_appendf(result,
                                  "# all the tls objects' attribute\n");
    if ((ret = tls_cmd_serialize(result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }

    (void)lagopus_dstring_appendf(result,
                                  "# all the snmp objects' attribute\n");
    if ((ret = snmp_cmd_serialize(result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }

    (void)lagopus_dstring_appendf(result,
                                  "# all the namespace objects' attribute\n");
    if ((ret = namespace_cmd_serialize(result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }

    /*
     * And after added all the non-object serializer, delete above.
     */

    if ((ret = s_create_all_objs_list(&ol)) >= 0) {
      size_t i, j;
      objtbl_t o;
      void **objs = NULL;
      size_t n_objs = 0;
      size_t n_ol = (size_t)ret;
      void *obj = NULL;
      const char *objname = NULL;
      bool objstate;

      /*
       * Then serialize all the objects' attribute.
       */
      for (i = 0; i < n_ol; i++) {
        o = ol[i].m_o;
        objs = ol[i].m_objs;
        n_objs = ol[i].m_n_objs;

        if (o != NULL &&
            (ret = lagopus_dstring_appendf(result,
                                           "# all the %s objects' attribute\n",
                                           o->m_name)) == LAGOPUS_RESULT_OK &&
            o->m_srz_proc != NULL &&
            n_objs > 0 && objs != NULL) {
          for (j = 0; j < n_objs; j++) {
            if ((ret = o->m_srz_proc(iptr, (*iptr)->m_status,
                                     objs[j], result)) != LAGOPUS_RESULT_OK) {
              goto end;
            }
          }
        }

        if ((ret = lagopus_dstring_appendf(result, "\n")) !=
            LAGOPUS_RESULT_OK) {
          goto end;
        }
      }

      /*
       * Then serialiize all the objects' status in normal order.
       */
      for (i = 0; i < n_ol; i++) {
        o = ol[i].m_o;
        objs = ol[i].m_objs;
        n_objs = ol[i].m_n_objs;

        if (o != NULL &&
            (ret = lagopus_dstring_appendf(result,
                                           "# %s objects' status\n",
                                           o->m_name)) == LAGOPUS_RESULT_OK &&
            o->m_ebl_proc != NULL && o->m_gnm_proc != NULL &&
            n_objs > 0 && objs != NULL) {
          for (j = 0; j < n_objs; j++) {
            obj = ol[i].m_objs[j];
            objname = NULL;
            objstate = false;
            if (obj != NULL &&
                (ret = o->m_gnm_proc(obj, &objname)) == LAGOPUS_RESULT_OK &&
                (ret = o->m_ebl_proc(iptr, (*iptr)->m_status,
                                     obj, &objstate, false, NULL)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, "%s %s %s\n",
                                                 o->m_name, objname,
                                                 (objstate == true) ?
                                                 "enable" : "disable")) !=
                  LAGOPUS_RESULT_OK) {
                goto end;
              }
            }
          }
        }

        if ((ret = lagopus_dstring_appendf(result, "\n")) !=
            LAGOPUS_RESULT_OK) {
          goto end;
        }
      }

      ret = LAGOPUS_RESULT_OK;

    end:
      s_destroy_all_objs_list(ol);
#ifdef NEED_CLEANUP_WHEN_SERIALIZE_FAILURE
      if (ret != LAGOPUS_RESULT_OK) {
        (void)lagopus_dstring_clear(result);
        (void)datastore_json_result_setf(result, ret, "\"\"");
      }
#endif /* NEED_CLEANUP_WHEN_SERIALIZE_FAILURE */
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline lagopus_result_t
s_atomic_auto_save(datastore_interp_t *iptr,
                   const char file[],
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  mode_t old_mask;
  int fd;

  /* save the current obj. */
  s_unlink_atomic_auto_save_file(iptr);

  strncpy((*iptr)->m_atomic_auto_save_file, file,
          sizeof((*iptr)->m_atomic_auto_save_file) - 1);
  old_mask = umask(S_IRUSR | S_IWUSR);
  fd = mkstemp((*iptr)->m_atomic_auto_save_file);
  (void) umask(old_mask);
  if (fd >= 0) {
    lagopus_msg_debug(1, "auto save file: %s\n",
                      (*iptr)->m_atomic_auto_save_file);
    close(fd);

    /* save. */
    ret = datastore_interp_save_file(
        iptr,
        (const char*) (*iptr)->m_current_configurator,
        (*iptr)->m_atomic_auto_save_file,
        result);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    lagopus_perror(ret);
  }

  return ret;
}

static inline lagopus_result_t
s_atomic_auto_load(datastore_interp_t *iptr,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* restore state. */
  s_restore_interp_state(*iptr);

  if ((ret = datastore_interp_destroy_obj(iptr, NULL, result)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("auto rollbacking from load file.\n");
    lagopus_msg_debug(1, "auto load file: %s\n",
                      (*iptr)->m_atomic_auto_save_file);
    (void)lagopus_dstring_clear(result);

    /* load. */
    ret = datastore_interp_load_file(
        iptr,
        (const char*) (*iptr)->m_current_configurator,
        (*iptr)->m_atomic_auto_save_file,
        result);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
    }
  } else {
    lagopus_perror(ret);
    lagopus_msg_error("CAUTION: Failed to rollback (auto load).\n");
  }

  return ret;
}

static inline lagopus_result_t
s_atomic_begin(datastore_interp_t *iptr,
               const char file[],
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    if ((*iptr)->m_status == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
      /*
       * Save the current state.
       */
      s_save_interp_state(*iptr);

      if ((ret = s_atomic_auto_save(iptr, file, result)) ==
          LAGOPUS_RESULT_OK) {
        /*
         * Then set the current state to atomic mode.
         */
        s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_ATOMIC);
      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_update_all_objs(datastore_interp_t *iptr,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    objlist_t *ol = NULL;
    if ((ret = s_create_all_objs_list(&ol)) > 0) {
      size_t i, j;
      objtbl_t o;
      void **objs = NULL;
      size_t n_objs = 0;
      size_t n_ol = (size_t)ret;

      (void)lagopus_dstring_clear(result);

      for (i = 0; i < n_ol; i++) {
        /*
         * The scan-build preaches the line. Don't care.
         */
        o = ol[i].m_o;
        objs = ol[i].m_objs;
        n_objs = ol[i].m_n_objs;
        if (o != NULL && o->m_upd_proc != NULL &&
            n_objs > 0 && objs != NULL) {
          for (j = 0; j < n_objs; j++) {
            if ((ret = o->m_upd_proc(iptr, (*iptr)->m_status,
                                     objs[j], result)) != LAGOPUS_RESULT_OK) {
              goto end;
            }
          }
        } else {
          /* object in the table is empty. */
          ret = LAGOPUS_RESULT_OK;
        }
      }
    end:
      s_destroy_all_objs_list(ol);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_atomic_abort(datastore_interp_t *iptr, lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    if ((*iptr)->m_status == DATASTORE_INTERP_STATE_ATOMIC) {
      (void)lagopus_dstring_clear(result);

      lagopus_msg_info("abort start.\n");

      s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_ABORTING);
      if ((ret = s_update_all_objs(iptr, result)) == LAGOPUS_RESULT_OK) {
        s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_ABORTED);
        if ((ret = s_update_all_objs(iptr, result)) == LAGOPUS_RESULT_OK) {
          lagopus_msg_info("abort success.\n");
        } else {
          lagopus_perror(ret);
          lagopus_msg_error("CAUTION: Failed to cleanup after abort.\n");
        }
      } else {
        lagopus_perror(ret);
        lagopus_msg_error("CAUTION: Failed to abort.\n");
      }

      /* unlink save file. */
      s_unlink_atomic_auto_save_file(iptr);

      s_restore_interp_state(*iptr);
    } else {
      ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_atomic_rollback(datastore_interp_t *iptr,
                  lagopus_dstring_t *result, bool force) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    if ((*iptr)->m_status == DATASTORE_INTERP_STATE_COMMIT_FAILURE ||
        ((*iptr)->m_status == DATASTORE_INTERP_STATE_ATOMIC && force == true)) {
      (void)lagopus_dstring_clear(result);

      lagopus_msg_info("rollback start.\n");

      s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_ROLLBACKING);
      if ((ret = s_update_all_objs(iptr, result)) == LAGOPUS_RESULT_OK) {
        s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_ROLLBACKED);
        if ((ret = s_update_all_objs(iptr, result)) == LAGOPUS_RESULT_OK) {
          lagopus_msg_info("rollback success.\n");
        } else {
          lagopus_perror(ret);
          lagopus_msg_error("CAUTION: Failed to cleanup after rollback.\n");
          ret = s_atomic_auto_load(iptr, result);
        }
      } else {
        lagopus_perror(ret);
        lagopus_msg_error("CAUTION: Failed to rollback.\n");
        ret = s_atomic_auto_load(iptr, result);
      }

      /* unlink save file. */
      s_unlink_atomic_auto_save_file(iptr);

      s_restore_interp_state(*iptr);
    } else {
      ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_atomic_commit(datastore_interp_t *iptr, lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    if ((*iptr)->m_status == DATASTORE_INTERP_STATE_ATOMIC) {

      (void)lagopus_dstring_clear(result);

      lagopus_msg_info("commit start.\n");

      s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_COMMITING);
      if ((ret = s_update_all_objs(iptr, result)) == LAGOPUS_RESULT_OK) {
        s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_COMMITED);
        if ((ret = s_update_all_objs(iptr, result)) == LAGOPUS_RESULT_OK) {
          lagopus_msg_info("commit success.\n");
          s_restore_interp_state(*iptr);
        } else {
          char *errstr = NULL;

          s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_COMMIT_FAILURE);

          lagopus_perror(ret);
          (void)lagopus_dstring_str_get(result, &errstr);
          if (IS_VALID_STRING(errstr) == true) {
            lagopus_msg_error("CAUTION: Failed to cleanup after commit: %s\n",
                              errstr);
          } else {
            lagopus_msg_error("CAUTION: Failed to cleanup after commit.\n");
          }
          free((void *)errstr);

          /* rollback here */
          ret = s_atomic_rollback(iptr, result, false);
        }
      } else {
        lagopus_result_t rbret = LAGOPUS_RESULT_ANY_FAILURES;
        char *rbresult = NULL;

        lagopus_perror(ret);
        lagopus_msg_error("CAUTION: Failed to commit.\n");

        s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_COMMIT_FAILURE);

        rbret = lagopus_dstring_str_get(result, &rbresult);
        if (rbret == LAGOPUS_RESULT_OK) {
          /* rollback here */
          rbret = s_atomic_rollback(iptr, result, false);
          if (rbret == LAGOPUS_RESULT_OK) {
            /* set commit error result. */
            (void)lagopus_dstring_clear(result);
            rbret = lagopus_dstring_appendf(result, "%s", rbresult);

            if (rbret != LAGOPUS_RESULT_OK) {
              lagopus_perror(rbret);
              ret = rbret;
            }
          } else {
            ret = rbret;
          }
        } else {
          lagopus_perror(rbret);
          ret = rbret;
        }
        free((void *) rbresult);
      }

      /* unlink save file. */
      s_unlink_atomic_auto_save_file(iptr);
    } else {
      ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_dryrun_begin(datastore_interp_t *iptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    if ((*iptr)->m_status == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
      /*
       * Save the current state.
       */
      s_save_interp_state(*iptr);
      s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_DRYRUN);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_dryrun_end(datastore_interp_t *iptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    if ((*iptr)->m_status == DATASTORE_INTERP_STATE_DRYRUN) {
      s_restore_interp_state(*iptr);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_add_cmd(datastore_interp_t ip,
          const char *argv0,
          datastore_command_proc_t proc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ip != NULL &&
      IS_VALID_STRING(argv0) == true &&
      proc != NULL) {
    void *val = (void *)proc;
    ret = lagopus_hashmap_add(&(ip->m_cmd_tbl), (void *)argv0, &val, false);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline datastore_command_proc_t
s_find_cmd(datastore_interp_t ip, const char *argv0) {
  datastore_command_proc_t ret = NULL;

  if (ip != NULL &&
      IS_VALID_STRING(argv0) == true) {
    void *val;
    if (lagopus_hashmap_find(&(ip->m_cmd_tbl), (void *)argv0, &val) ==
        LAGOPUS_RESULT_OK) {
      ret = (datastore_command_proc_t)val;
    }
  }

  return ret;
}





static inline void
s_lock_cnf(void) {
  (void)lagopus_mutex_lock(&(s_lock_lock));
}


static inline void
s_unlock_cnf(void) {
  (void)lagopus_mutex_unlock(&(s_lock_lock));
}


static inline lagopus_result_t
s_add_cnf(const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(name) == true) {
    configurator_t c = (configurator_t)malloc(sizeof(*c));
    if (c != NULL) {
      const char *cpname = strdup(name);
      if (IS_VALID_STRING(cpname) == true) {
        void *val = (void *)c;
        c->m_name = cpname;
        c->m_has_lock = false;
        if ((ret = lagopus_hashmap_add(&s_cnf_tbl, (void *)(c->m_name),
                                       &val, false)) != LAGOPUS_RESULT_OK) {
          free((void *)(c->m_name));
          free((void *)c);
        }
      } else {
        free((void *)cpname);
        free((void *)c);
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_delete_cnf(const char *name) {
  if (IS_VALID_STRING(name) == true) {
    (void)lagopus_hashmap_delete(&s_cnf_tbl, (void *)name, NULL, true);
  }
}


static inline configurator_t
s_find_cnf(const char *name) {
  configurator_t ret = NULL;

  if (IS_VALID_STRING(name) == true) {
    void *val;
    if (lagopus_hashmap_find(&s_cnf_tbl, (void *)name, &val) ==
        LAGOPUS_RESULT_OK) {
      ret = (configurator_t)val;
    }
  }

  return ret;
}


static inline lagopus_result_t
s_has_cnf_lock(const char *name, bool *bptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(name) == true && bptr != NULL) {
    configurator_t c = s_find_cnf(name);
    *bptr = false;

    if (c != NULL) {
      *bptr = c->m_has_lock;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_cnf_lock(const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(name) == true) {
    if (s_lock_holder == NULL) {
      configurator_t c = s_find_cnf(name);

      if (c != NULL) {
        if (c->m_has_lock == false) {
          s_lock_holder = c;
          c->m_has_lock = true;
          ret = LAGOPUS_RESULT_OK;
        } else {
          lagopus_exit_fatal("Invalid lock holder '%s'. "
                             "The lock is about to be "
                             "acquired by '%s' : Must not happen.\n",
                             c->m_name, name);
          /* not reached. */
        }
      } else {
        ret = LAGOPUS_RESULT_NOT_FOUND;
      }
    } else {
      if (s_lock_holder->m_has_lock == true) {
        if (strcmp(s_lock_holder->m_name, name) == 0) {
          /*
           * the configurator 'name' already has the lock. Just treat
           * this as a OK.
           */
          ret = LAGOPUS_RESULT_OK;
        } else {
          /*
           * The lock is already acquired by another configurator.
           */
          ret = LAGOPUS_RESULT_BUSY;
        }
      } else {
        lagopus_exit_fatal("Invalid lock holder '%s'. The lock is about to be "
                           "acquired by '%s' : Must not happen.\n",
                           s_lock_holder->m_name, name);
        /* not reached. */
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_cnf_unlock(const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(name) == true) {
    if (s_lock_holder != NULL) {
      if (s_lock_holder->m_has_lock == true) {
        if (strcmp(s_lock_holder->m_name, name) == 0) {
          s_lock_holder->m_has_lock = false;
          s_lock_holder = NULL;
          ret = LAGOPUS_RESULT_OK;
        } else {
          /*
           * The lock is already acquired by another configurator.
           */
          ret = LAGOPUS_RESULT_NOT_OWNER;
        }
      } else {
        lagopus_exit_fatal("Configurator '%s' has no lock. "
                           "The lock is about to be "
                           "released by '%s' : Must not happen.\n",
                           s_lock_holder->m_name, name);
        /* not reached. */
      }
    } else {
      /*
       * No one has acquired the lock. Just treat this as a OK.
       */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline void
s_lock_interp(datastore_interp_t ip) {
  if (ip != NULL && ip->m_lock != NULL) {
    (void)lagopus_mutex_lock(&(ip->m_lock));
  }
}


static inline void
s_unlock_interp(datastore_interp_t ip) {
  if (ip != NULL && ip->m_lock != NULL) {
    (void)lagopus_mutex_unlock(&(ip->m_lock));
  }
}


static inline bool
s_is_interp_locked(datastore_interp_t ip) {
  bool ret = false;
  if (ip != NULL && ip->m_lock != NULL) {
    lagopus_result_t r = lagopus_mutex_trylock(&(ip->m_lock));
    if (r == LAGOPUS_RESULT_OK) {
      (void)lagopus_mutex_unlock(&(ip->m_lock));
    } else if (r == LAGOPUS_RESULT_BUSY) {
      ret = true;
    }
  }
  return ret;
}


static inline bool
s_is_interp_functional(datastore_interp_t ip) {
  bool ret = false;
  if (ip != NULL && ip->m_cmd_tbl != NULL &&
      lagopus_hashmap_size(&(ip->m_cmd_tbl)) > 0) {
    ret = true;
  }
  return ret;
}


static inline void
s_set_interp_state(datastore_interp_t ip, datastore_interp_state_t s) {
  if (ip != NULL) {
    ip->m_status = s;
  }
}


static inline datastore_interp_state_t
s_get_interp_state(datastore_interp_t ip) {
  datastore_interp_state_t ret = DATASTORE_INTERP_STATE_UNKNOWN;

  if (ip != NULL) {
    ret = ip->m_status;
  }

  return ret;
}


static inline void
s_save_interp_state(datastore_interp_t ip) {
  if (ip != NULL) {
    ip->m_saved_status = ip->m_status;
  }
}


static inline void
s_restore_interp_state(datastore_interp_t ip) {
  if (ip != NULL) {
    ip->m_status = ip->m_saved_status;
  }
}


static inline bool
s_is_interp_interactive(datastore_interp_t ip) {
  if (ip != NULL) {
    return ip->m_is_stream;
  } else {
    return false;
  }
}





static char *
s_session_gets(char *s, int size, void *stream) {
  return session_fgets(s, size, (lagopus_session_t)stream);
}


static char *
s_FILE_gets(char *s, int size, void *stream) {
  return fgets(s, size, (FILE *)stream);
}


static int
s_session_printf(void *stream, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  va_end(args);

  return session_vprintf((lagopus_session_t)stream, fmt, args);
}


static int
s_FILE_printf(void *stream, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  va_end(args);

  return vfprintf((FILE *)stream, fmt, args);
}


static inline void
s_linereader_init(datastore_interp_t ip,
                  void *stream_in, void *stream_out,
                  datastore_gets_proc_t in_proc,
                  datastore_printf_proc_t out_proc) {
  if (ip != NULL) {
    lagopus_dstring_clear(&(ip->m_lr_ctx.m_buf));
    ip->m_lr_ctx.m_stream_in = stream_in;
    ip->m_lr_ctx.m_stream_out = stream_out;
    ip->m_lr_ctx.m_gets_proc = in_proc;
    ip->m_lr_ctx.m_printf_proc = out_proc;
    ip->m_lr_ctx.m_lineno = 0;
    ip->m_lr_ctx.m_got_EOF = false;
  }
}


static inline void
s_linereader_final(datastore_interp_t ip) {
  if (ip != NULL) {
    lagopus_dstring_clear(&(ip->m_lr_ctx.m_buf));
    ip->m_lr_ctx.m_stream_in = NULL;
    ip->m_lr_ctx.m_stream_out = NULL;
    ip->m_lr_ctx.m_gets_proc = NULL;
    ip->m_lr_ctx.m_printf_proc = NULL;
    ip->m_lr_ctx.m_lineno = 0;
    ip->m_lr_ctx.m_got_EOF = false;
  }
}


static inline bool
s_linereader_got_EOF(datastore_interp_t ip) {
  bool ret = false;

  if (ip != NULL) {
    ret = ip->m_lr_ctx.m_got_EOF;
  }

  return ret;
}


static inline void
s_linereader_set_filename(datastore_interp_t ip, const char *filename) {
  if (ip != NULL && IS_VALID_STRING(filename) == true) {
    ip->m_lr_ctx.m_filename = filename;
  }
}


static inline void
s_linereader_reset_filename(datastore_interp_t ip) {
  s_linereader_set_filename(ip, NULL);
}


static inline lagopus_result_t
s_getline(datastore_interp_t ip,
          void *stream, char **line, size_t *lineno) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ip != NULL && stream != NULL && line != NULL) {
    char *c = NULL;

    if (lineno != NULL) {
      *lineno = 0;
    }

    if (ip->m_lr_ctx.m_gets_proc != NULL) {
      char buf[65536];
      size_t len;
      char *a_line = NULL;

      lagopus_dstring_clear(&(ip->m_lr_ctx.m_buf));

      while ((c = ip->m_lr_ctx.m_gets_proc(buf,
                                           sizeof(buf), stream)) != NULL) {
        if (a_line != NULL) {
          free((void *)a_line);
        }
        a_line = NULL;
        ip->m_lr_ctx.m_lineno++;

        len = strlen(buf);
        if ((int)(buf[len - 1]) == '\r' ||
            (int)(buf[len - 1]) == '\n') {
          if ((ret = lagopus_str_trim_right((const char *)buf, " \t\r\n",
                                            &a_line)) > 0) {
            if ((int)(a_line[ret - 1]) == '\\') {
              a_line[ret - 1] = '\0';
              len = strlen(a_line);
              if (len > 0) {
                if ((ret = lagopus_dstring_appendf(&(ip->m_lr_ctx.m_buf),
                                                   "%s", a_line)) ==
                    LAGOPUS_RESULT_OK) {
                  /*
                   * Appended. Continue to read.
                   */
                  continue;
                } else {
                  /*
                   * Append failure. Bailout.
                   */
                  break;
                }
              }
            } else {
              /*
               * Got a line. Done.
               */
              ret = lagopus_dstring_appendf(&(ip->m_lr_ctx.m_buf),
                                            "%s", a_line);
              break;
            }
          } else if (ret == 0) {
            continue;
          } else {
            break;
          }
        } else {
          /*
           * The string is not terminated by '\r' nor '\n'. Continue.
           */
          if ((ret = lagopus_dstring_appendf(&(ip->m_lr_ctx.m_buf),
                                             "%s", buf)) ==
              LAGOPUS_RESULT_OK) {
            /*
             * Appended. Continue to read.
             */
            continue;
          } else {
            /*
             * Append failure. Bailout.
             */
            break;
          }
        }
      }

      if (a_line != NULL) {
        free((void *)a_line);
      }

      if (c == NULL) {
        /*
         * Got an EOF.
         */
        ip->m_lr_ctx.m_got_EOF = true;
        ret = LAGOPUS_RESULT_OK;
      } else if (ret == LAGOPUS_RESULT_OK) {
        ret = lagopus_dstring_str_get(&(ip->m_lr_ctx.m_buf), line);
      }
    }

    if (lineno != NULL) {
      *lineno = ip->m_lr_ctx.m_lineno;
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline bool
s_is_blocking_session(datastore_interp_t *iptr) {
  datastore_config_type_t ftype = DATASTORE_CONFIG_TYPE_UNKNOWN;
  bool ret = false;
  session_info_t *si;
  void *stream_out = NULL;
  uint64_t session_id = 0LL;
  bool b = false;

  if (datastore_interp_get_current_file_context(iptr,
      NULL, NULL,
      NULL, &stream_out,
      NULL, NULL,
      &ftype) ==
      LAGOPUS_RESULT_OK &&
      ftype == DATASTORE_CONFIG_TYPE_STREAM_SESSION &&
      stream_out != NULL) {
    session_id = session_id_get((struct session *) stream_out);
    if (lagopus_hashmap_find(&((*iptr)->m_blocking_sessions),
                             (void *) session_id,
                             (void **) &si) ==
        LAGOPUS_RESULT_OK) {

      if (stream_out == si->session &&
          lagopus_thread_is_valid(si->thd, &b) ==
          LAGOPUS_RESULT_OK &&
          b == true) {
        ret = true;
      } else {
        datastore_interp_blocking_session_unset(
          iptr, (struct session *) stream_out);
      }
    }
  }

  return ret;
}

static inline lagopus_result_t
s_eval_str(datastore_interp_t *iptr, const char *str, lagopus_dstring_t *ds,
           void *stream_out) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && IS_VALID_STRING(str) == true) {
    char *cs = strdup(str);

    if (IS_VALID_STRING(cs) == true) {
      char *cmds[TOKEN_MAX + 1];
      char *errcmd = NULL;
      lagopus_result_t n_cmds = lagopus_str_tokenize(cs, cmds, TOKEN_MAX + 1,
                                "\r\n");

      if (n_cmds > 0 && n_cmds <= TOKEN_MAX) {
        size_t i;
        char *tokens[TOKEN_MAX + 1];
        lagopus_result_t n_tokens;
        datastore_command_proc_t cmdproc;
        objtbl_t objtbl;

        (void)memset((void *)tokens, 0, sizeof(tokens));
        for (i = 0; i < (size_t)n_cmds; i++) {
          /* Now sending large data. */
          if (s_is_blocking_session(iptr) == true) {
            lagopus_msg_warning("Now sending large data. Skip cmd (%s).",
                                cmds[i]);
            ret = LAGOPUS_RESULT_OK;
            continue;
          }

          n_tokens = lagopus_str_tokenize_quote(cmds[i], tokens, TOKEN_MAX + 1,
                                                " \t\r\n", "\"'");

          if (n_tokens == 0) {
            ret = LAGOPUS_RESULT_OK;
            continue;
          } else if (n_tokens == LAGOPUS_RESULT_QUOTE_NOT_CLOSED &&
                     *(tokens[0]) == '#') {
            ret = LAGOPUS_RESULT_OK;
            continue;
          } else if (n_tokens > 0 && n_tokens <= TOKEN_MAX) {
            if (*(tokens[0]) == '#') {
              ret = LAGOPUS_RESULT_OK;
              continue;
            }

            cmdproc = s_find_cmd(*iptr, tokens[0]);
            objtbl = s_find_tbl(tokens[0]);
            if (cmdproc != NULL) {
              ret = cmdproc(iptr, (*iptr)->m_status,
                            (size_t)n_tokens, (const char * const *)tokens,
                            (objtbl != NULL) ? objtbl->m_hptr : NULL,
                            (objtbl != NULL) ? objtbl->m_upd_proc : NULL,
                            (objtbl != NULL) ? objtbl->m_ebl_proc : NULL,
                            (objtbl != NULL) ? objtbl->m_srz_proc : NULL,
                            (objtbl != NULL) ? objtbl->m_dty_proc : NULL,
                            ds);
              if (ret != LAGOPUS_RESULT_OK) {
                break;
              }
            } else {
              /*
               * The scan-build preaches the line below. Don't care.
               */
              if ((*iptr)->m_status != DATASTORE_INTERP_STATE_PRELOAD) {
                ret = LAGOPUS_RESULT_NOT_FOUND;
                errcmd = strdup(tokens[0]);
                break;
              } else {
                /*
                 * Don't treat this case as an error. This is for
                 * pre-loading, in which all the commands are not
                 * registered yet.
                 */
                ret = LAGOPUS_RESULT_OK;
                continue;
              }
            }
          } else {
            if (n_tokens <= 0) {
              ret = n_tokens;
            } else {
              ret = LAGOPUS_RESULT_TOO_MANY_OBJECTS;
            }
            break;
          }
        }

      } else {
        if (n_cmds <= 0) {
          ret = n_cmds;
        } else {
          ret = LAGOPUS_RESULT_TOO_MANY_OBJECTS;
        }
      }

      free((void *)cs);

      switch (ret) {
        case LAGOPUS_RESULT_NOT_FOUND: {
          if (IS_VALID_STRING(errcmd) == true) {
            (void)datastore_json_result_string_setf(ds, ret,
                                                    "'%s' command not found.",
                                                    errcmd);
          } else {
            (void)datastore_json_result_string_setf(ds, ret,
                                                    "command not found.");
          }
          break;
        }
        case LAGOPUS_RESULT_TOO_MANY_OBJECTS: {
          (void)datastore_json_result_string_setf(ds, ret,
                                                  "Too many lines or tokens.");
          break;
        }
        default: {
          /*
           * Don't overwrite/modify result string any other cases.
           */
          break;
        }
      }

      if (ds != NULL &&
          stream_out != NULL &&
          s_is_interp_interactive(*iptr) == true) {
        /*
         * Output result if desired.
         */
        char *result = NULL;
        if (lagopus_dstring_str_get(ds, &result) ==
            LAGOPUS_RESULT_OK &&
            IS_VALID_STRING(result)) {
          (void)((*iptr)->m_lr_ctx.m_printf_proc)(stream_out, "%s\n", result);
          free((void *)result);
          result = NULL;
        }
      }

      free((void *)errcmd);

    } else {
      free((void *)cs);
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_eval_stream(datastore_interp_t *iptr,
              void *stream_in, datastore_gets_proc_t getsproc,
              void *stream_out, datastore_printf_proc_t printproc,
              lagopus_dstring_t *ds) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && stream_in != NULL &&
      (((getsproc == s_session_gets || getsproc == s_FILE_gets) &&
        (printproc == s_session_printf || printproc == s_FILE_printf)) ||
       ((getsproc == s_session_gets || getsproc == s_FILE_gets) &&
        (printproc == NULL || stream_out == NULL)))) {

    if ((*iptr)->m_cmd_string == NULL) {
      char *line = NULL;

      s_linereader_init(*iptr, stream_in, stream_out,
                        getsproc, printproc);
      if (s_is_interp_interactive(*iptr) == true) {

        /*
         * The interactive mode. Get a line, evaluate, and return.
         */
        if ((ret = s_getline(*iptr,
                             stream_in, &line, NULL)) == LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(line) == true) {
            ret = s_eval_str(iptr, (const char *)line, ds, stream_out);
          } else {
            ret = (s_linereader_got_EOF(*iptr) == false) ?
                LAGOPUS_RESULT_OK : LAGOPUS_RESULT_EOF;
          }
        }

      } else {

        size_t lineno = 0;

        /*
         * The "config file load" mode. Evaluate lines until got an EOF.
         */
        while (s_linereader_got_EOF(*iptr) == false) {
          if (line != NULL) {
            free((void *)line);
            line = NULL;
          }
          if ((ret = s_getline(*iptr, stream_in, &line, &lineno)) ==
              LAGOPUS_RESULT_OK) {
            if (IS_VALID_STRING(line) == true) {
              if ((ret = s_eval_str(iptr, (const char *)line, ds, stream_out)) ==
                  LAGOPUS_RESULT_OK) {
                continue;
              } else {
                /*
                 * return line # in ds.
                 */
                size_t len = 0;
                if ((len = (size_t) lagopus_dstring_len_get(ds)) >
                    LAGOPUS_RESULT_OK) {
                  if (lagopus_dstring_insertf(ds,
                                              len - 1,
                                              ", \"line\": %zu",
                                              lineno) == LAGOPUS_RESULT_OK) {
                  } else {
                    lagopus_msg_warning("message create failed.\n");
                  }
                } else {
                  lagopus_msg_warning("message create failed.\n");
                }

                break;
              }
            } else {
              ret = LAGOPUS_RESULT_OK;
              if (s_linereader_got_EOF(*iptr) == false) {
                continue;
              } else {
                break;
              }
            }
          } else {
            break;
          }
        }

      }
      free((void *)line);
      s_linereader_final(*iptr);
    } else {
      ret = s_eval_str(iptr, (const char *) (*iptr)->m_cmd_string, ds, stream_out);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_eval_stream_wrap(datastore_interp_t *iptr,
                   const char *name,
                   bool is_stream,
                   void *stream_in, datastore_gets_proc_t getsproc,
                   void *stream_out, datastore_printf_proc_t printproc,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && IS_VALID_STRING(name) == true &&
      stream_in != NULL) {
    if (s_find_cnf(name) != NULL) {
      bool o_is_stream = (*iptr)->m_is_stream;
      volatile const char *o_name = (*iptr)->m_current_configurator;

      (*iptr)->m_is_stream = is_stream;
      (*iptr)->m_current_configurator = name;
      ret = s_eval_stream(iptr,
                          stream_in, getsproc,
                          stream_out, printproc,
                          result);
      /*
       * The scan-build preaches the line below. Don't care.
       */
      (*iptr)->m_current_configurator = o_name;
      (*iptr)->m_is_stream = o_is_stream;

    } else {
      ret = LAGOPUS_RESULT_NOT_ALLOWED;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_eval_file(datastore_interp_t *iptr,
            const char *name,
            const char *file,
            bool do_preload,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && IS_VALID_STRING(name) == true &&
      IS_VALID_STRING(file) == true) {
    FILE *fd = NULL;

    errno = 0;
    fd = fopen(file, "r");
    if (fd != NULL) {

      linereader_context lrctx = (*iptr)->m_lr_ctx;
      datastore_interp_state_t ostate = (*iptr)->m_status;

      if (do_preload == true) {
        (*iptr)->m_status = DATASTORE_INTERP_STATE_PRELOAD;
      }

      s_linereader_set_filename(*iptr, file);
      ret = s_eval_stream_wrap(iptr, name, false,
                               (void *)fd, s_FILE_gets,
                               NULL, NULL,
                               result);
      s_linereader_reset_filename(*iptr);

      (void)fclose(fd);

      if (do_preload == true) {
        (*iptr)->m_status = ostate;
      }

      if (ret != LAGOPUS_RESULT_OK) {
        /*
         * return a filename in result.
         */
        char *escaped_file = NULL;
        size_t len = 0;

        if (lagopus_str_escape(file,
                               "\"'/",
                               false,
                               &escaped_file) == LAGOPUS_RESULT_OK) {
          if ((len = (size_t) lagopus_dstring_len_get(result)) >
              LAGOPUS_RESULT_OK) {
            if (lagopus_dstring_insertf(result,
                                        len - 1,
                                        ", \"file\": \"%s\"",
                                        escaped_file) == LAGOPUS_RESULT_OK) {
            } else {
              lagopus_msg_warning("message create failed.\n");
            }
          } else {
            lagopus_msg_warning("message create failed.\n");
          }
        } else {
          lagopus_msg_warning("filename escape failed.\n");
        }

        free(escaped_file);
      }

      (*iptr)->m_lr_ctx = lrctx;

    } else {
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_destroy_obj(datastore_interp_t *iptr, const char *namespace,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  objlist_t *ol = NULL;
  char *target = NULL;

  if (iptr != NULL && *iptr != NULL && result != NULL) {
    if ((ret = s_create_all_objs_list(&ol)) >= 0) {
      size_t i, j;
      objtbl_t o;
      void **objs = NULL;
      size_t n_objs = 0;
      size_t n_ol = (size_t)ret;
      size_t delim_len = 0;
      size_t target_len = 0;
      const char *objname = NULL;

      if (namespace != NULL) {
        delim_len = strlen(DATASTORE_NAMESPACE_DELIMITER);

        if (namespace[0] == '\0') {
          target_len = delim_len;
        } else {
          target_len = strlen(namespace) + delim_len;
        }

        target = (char *) malloc(sizeof(char) * (target_len + 1));

        if (target != NULL) {
          ret = snprintf(target, target_len + 1, "%s%s",
                         namespace, DATASTORE_NAMESPACE_DELIMITER);
          if (ret < 0) {
            lagopus_msg_warning("target namespace create failed.\n");
            ret = LAGOPUS_RESULT_ANY_FAILURES;
            goto end;
          }
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
          goto end;
        }
      }

      /*
       * destroy from the root object.
       */
      for (i = n_ol; i > 0; i--) {
        o = ol[i - 1].m_o;
        objs = ol[i - 1].m_objs;
        n_objs = ol[i - 1].m_n_objs;

        if (o != NULL && o->m_dty_proc != NULL &&
            n_objs > 0 && objs != NULL) {

          for (j = n_objs; j > 0; j--) {
            if (target != NULL) {
              ret = o->m_gnm_proc(objs[j - 1], &objname);
              if (ret == LAGOPUS_RESULT_OK) {
                if (strstr(objname, target) == NULL) {
                  /* namespace not matched. */
                  continue;
                }
              } else {
                lagopus_msg_warning("get object name failed.\n");
                goto end;
              }

              lagopus_msg_debug(10, "destroy object name: %s\n", objname);
            }

            if ((ret = o->m_dty_proc(iptr, (*iptr)->m_status,
                                     objs[j - 1],
                                     result)) != LAGOPUS_RESULT_OK) {
              goto end;
            }
          }
        }
      }

      ret = LAGOPUS_RESULT_OK;
    }

  end:
    s_destroy_all_objs_list(ol);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  free(target);

  return ret;
}


static inline lagopus_result_t
s_duplicate_obj(datastore_interp_t *iptr, const char *cur_namespace,
                const char *dup_namespace, lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  objlist_t *ol = NULL;
  char *target = NULL;

  if (iptr != NULL && *iptr != NULL && result != NULL &&
      cur_namespace != NULL && dup_namespace != NULL) {
    if ((ret = s_create_all_objs_list(&ol)) >= 0) {
      size_t i, j;
      objtbl_t o;
      void **objs = NULL;
      size_t n_objs = 0;
      size_t n_ol = (size_t)ret;
      size_t delim_len = 0;
      size_t target_len = 0;
      const char *objname = NULL;

      delim_len = strlen(DATASTORE_NAMESPACE_DELIMITER);

      if (cur_namespace[0] == '\0') {
        target_len = delim_len;
      } else {
        target_len = strlen(cur_namespace) + delim_len;
      }

      target = (char *) malloc(sizeof(char) * (target_len + 1));

      if (target != NULL) {
        ret = snprintf(target, target_len + 1, "%s%s",
                       cur_namespace, DATASTORE_NAMESPACE_DELIMITER);
        if (ret < 0) {
          lagopus_msg_warning("target namespace create failed.\n");
          ret = LAGOPUS_RESULT_ANY_FAILURES;
          goto end;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto end;
      }

      /*
       * duplicate the object if namespace matches.
       */
      for (i = n_ol; i > 0; i--) {
        o = ol[i - 1].m_o;
        objs = ol[i - 1].m_objs;
        n_objs = ol[i - 1].m_n_objs;

        if (o != NULL && o->m_dup_proc != NULL &&
            n_objs > 0 && objs != NULL) {

          for (j = n_objs; j > 0; j--) {
            ret = o->m_gnm_proc(objs[j - 1], &objname);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("get object name failed.\n");
              goto end;
            }

            if (strstr(objname, target) == NULL) {
              /* namespace not matched. */
              continue;
            }

            lagopus_msg_debug(10, "duplicate object name: %s\n", objname);

            if ((ret = o->m_dup_proc(objs[j - 1],
                                     dup_namespace)) != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("object duplicate failed.\n");
              goto end;
            }
          }
        }
      }

      ret = LAGOPUS_RESULT_OK;
    }

  end:
    s_destroy_all_objs_list(ol);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  free(target);

  return ret;
}





lagopus_result_t
datastore_register_table(const char *argv0,
                         lagopus_hashmap_t *tptr,
                         datastore_update_proc_t u_proc,
                         datastore_enable_proc_t e_proc,
                         datastore_serialize_proc_t s_proc,
                         datastore_destroy_proc_t d_proc,
                         datastore_compare_proc_t c_proc,
                         datastore_getname_proc_t n_proc,
                         datastore_duplicate_proc_t dup_proc) {
  return s_add_tbl(argv0,
                   tptr, u_proc, e_proc, s_proc, d_proc, c_proc, n_proc,
                   dup_proc);
}


lagopus_result_t
datastore_find_table(const char *argv0,
                     lagopus_hashmap_t *tptr,
                     datastore_update_proc_t *u_pptr,
                     datastore_enable_proc_t *e_pptr,
                     datastore_serialize_proc_t *s_pptr,
                     datastore_destroy_proc_t *d_pptr,
                     datastore_compare_proc_t *c_pptr,
                     datastore_getname_proc_t *n_pptr,
                     datastore_duplicate_proc_t *dup_pptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(argv0) == true) {
    objtbl_t o = s_find_tbl(argv0);

    if (o != NULL) {
      if (tptr != NULL) {
        *tptr = *(o->m_hptr);
      }
      if (u_pptr != NULL) {
        *u_pptr = o->m_upd_proc;
      }
      if (e_pptr != NULL) {
        *e_pptr = o->m_ebl_proc;
      }
      if (s_pptr != NULL) {
        *s_pptr = o->m_srz_proc;
      }
      if (d_pptr != NULL) {
        *d_pptr = o->m_dty_proc;
      }
      if (c_pptr != NULL) {
        *c_pptr = o->m_cmp_proc;
      }
      if (n_pptr != NULL) {
        *n_pptr = o->m_gnm_proc;
      }
      if (dup_pptr != NULL) {
        *dup_pptr = o->m_dup_proc;
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_get_objects(const char *argv0, void ***listp, bool do_sort) {
  return s_find_objs(argv0, listp, do_sort);
}





lagopus_result_t
datastore_create_interp(datastore_interp_t *iptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL) {
    datastore_interp_t ip = NULL;
    if (*iptr == NULL) {
      if ((ip = (datastore_interp_t)malloc(sizeof(*ip))) != NULL) {
        memset(ip, 0, sizeof(*ip));
        ip->m_is_allocd = true;
        *iptr = ip;
      } else {
        goto done;
      }
    } else {
      ip = *iptr;
      ip->m_is_allocd = false;
    }
    if (ip != NULL) {
      if ((ret = lagopus_mutex_create(&(ip->m_lock))) == LAGOPUS_RESULT_OK &&
          (ret = lagopus_dstring_create(&(ip->m_lr_ctx.m_buf))) ==
          LAGOPUS_RESULT_OK &&
          (ret = lagopus_hashmap_create(&(ip->m_cmd_tbl),
                                        LAGOPUS_HASHMAP_TYPE_STRING,
                                        NULL)) ==
          LAGOPUS_RESULT_OK &&
          (ret = lagopus_hashmap_create(&(ip->m_blocking_sessions),
                                        LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                        s_session_info_free)) ==
          LAGOPUS_RESULT_OK) {
        ip->m_current_configurator = NULL;
        ip->m_status = DATASTORE_INTERP_STATE_AUTO_COMMIT;
        ip->m_saved_status = DATASTORE_INTERP_STATE_AUTO_COMMIT;
        ip->m_is_stream = false;
        ip->m_atomic_auto_save_file[0] = '\0';
        ip->m_cmd_string = NULL;
        s_linereader_init(ip, NULL, NULL, NULL, NULL);
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


lagopus_result_t
datastore_shutdown_interp(datastore_interp_t *iptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {

    s_lock_interp(*iptr);
    {
      if ((*iptr)->m_status != DATASTORE_INTERP_STATE_DESTROYING) {
        s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_SHUTDOWN);
      }
    }
    s_unlock_interp(*iptr);

    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}


void
datastore_destroy_interp(datastore_interp_t *iptr) {
  if (iptr != NULL && *iptr != NULL) {

    s_lock_interp(*iptr);
    {
      if ((*iptr)->m_status != DATASTORE_INTERP_STATE_SHUTDOWN) {
        s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_SHUTDOWN);
      }
      s_set_interp_state(*iptr, DATASTORE_INTERP_STATE_DESTROYING);
      lagopus_dstring_destroy(&((*iptr)->m_lr_ctx.m_buf));
      lagopus_hashmap_destroy(&((*iptr)->m_cmd_tbl),
                              true);
      lagopus_hashmap_destroy(&((*iptr)->m_blocking_sessions),
                              true);
      s_unlink_atomic_auto_save_file(iptr);
    }
    s_unlock_interp(*iptr);

    (void)lagopus_mutex_destroy(&((*iptr)->m_lock));

    if ((*iptr)->m_is_allocd == true) {
      free((void *)(*iptr));
    }
  }
}


lagopus_result_t
datastore_get_interp_state(const datastore_interp_t *iptr,
                           datastore_interp_state_t *sptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && sptr != NULL) {

    s_lock_interp(*iptr);
    {
      *sptr = s_get_interp_state(*iptr);
    }
    s_unlock_interp(*iptr);

    ret = LAGOPUS_RESULT_OK;

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





lagopus_result_t
datastore_register_configurator(const char *name) {
  return s_add_cnf(name);
}


void
datastore_unregister_configurator(const char *name) {
  s_delete_cnf(name);
}





lagopus_result_t
datastore_interp_register_command(datastore_interp_t *iptr,
                                  const char *name,
                                  const char *argv0,
                                  datastore_command_proc_t proc) {
  (void)name;
  return s_add_cmd(*iptr, argv0, proc);
}


lagopus_result_t
datastore_interp_eval_string(datastore_interp_t *iptr,
                             const char *name,
                             const char *str,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_functional(*iptr) == true &&
      IS_VALID_STRING(name) == true &&
      IS_VALID_STRING(str) == true) {
    if (s_find_cnf(name) != NULL) {

      s_lock_interp(*iptr);
      {
        (*iptr)->m_current_configurator = name;
        ret = s_eval_str(iptr, str, result, NULL);
        (*iptr)->m_current_configurator = NULL;
      }
      s_unlock_interp(*iptr);

    } else {
      ret = LAGOPUS_RESULT_NOT_ALLOWED;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_eval_fd(datastore_interp_t *iptr,
                         const char *name,
                         FILE *infd,
                         FILE *outfd,
                         bool to_eof,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_functional(*iptr) == true &&
      IS_VALID_STRING(name) == true) {

    s_lock_interp(*iptr);
    {
      ret = s_eval_stream_wrap(iptr, name, !to_eof,
                               (void *)infd, s_FILE_gets,
                               (void *)outfd, s_FILE_printf,
                               result);
    }
    s_unlock_interp(*iptr);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_eval_session(datastore_interp_t *iptr,
                              const char *name,
                              struct session *sptr,
                              bool to_eof,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_functional(*iptr) == true &&
      IS_VALID_STRING(name) == true) {

    s_lock_interp(*iptr);
    {
      ret = s_eval_stream_wrap(iptr, name, !to_eof,
                               (void *)sptr, s_session_gets,
                               (void *)sptr, s_session_printf,
                               result);
    }
    s_unlock_interp(*iptr);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_eval_file(datastore_interp_t *iptr,
                           const char *name,
                           const char *file,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_functional(*iptr) == true &&
      IS_VALID_STRING(name) == true &&
      IS_VALID_STRING(file) == true) {

    s_lock_interp(*iptr);
    {
      ret = s_eval_file(iptr, name, file, false, result);
    }
    s_unlock_interp(*iptr);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
datastore_interp_cancel_janitor(datastore_interp_t *iptr) {
  if (iptr != NULL && *iptr != NULL) {
    s_unlock_interp(*iptr);
  }
}





/*
 * datastore internal exported APIs
 */
lagopus_result_t
datastore_interp_eval_file_partial(datastore_interp_t *iptr,
                                   const char *name,
                                   const char *file,
                                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && IS_VALID_STRING(name) == true &&
      IS_VALID_STRING(file) == true) {

    s_lock_interp(*iptr);
    {
      ret = s_eval_file(iptr, name, file, true, result);
    }
    s_unlock_interp(*iptr);

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





/*
 * internal exported and lock-free APIs.
 * Note that the following APIs are supposed to be invoked from DSL
 * command parsers, not from other APIs directly.
 */
lagopus_result_t
datastore_interp_load_file(datastore_interp_t *iptr,
                           const char *name,
                           const char *file,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true &&
      IS_VALID_STRING(name) == true &&
      IS_VALID_STRING(file) == true) {
    ret = s_eval_file(iptr, name, file, false, result);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_serialize(datastore_interp_t *iptr, const char *name,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true &&
      IS_VALID_STRING(name) == true &&
      result != NULL) {
    if (s_find_cnf(name) != NULL) {
      volatile const char *o_name = (*iptr)->m_current_configurator;
      (*iptr)->m_current_configurator = name;
      ret = s_serialize(iptr, result);
      (*iptr)->m_current_configurator = o_name;
    } else {
      ret = LAGOPUS_RESULT_NOT_ALLOWED;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_save_file(datastore_interp_t *iptr,
                           const char *name,
                           const char *file,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true &&
      IS_VALID_STRING(name) == true &&
      IS_VALID_STRING(file) == true) {
    if (s_find_cnf(name) != NULL) {
      int err = 0;

      if ((err = s_is_writable_file(file)) == 0) {
        char *save_str = NULL;
        lagopus_dstring_t save_dstr = NULL;

        if (lagopus_dstring_create(&save_dstr) == LAGOPUS_RESULT_OK) {
          (void)lagopus_dstring_clear(&save_dstr);

          if ((ret = datastore_interp_serialize(iptr, name, &save_dstr)) ==
              LAGOPUS_RESULT_OK &&
              (ret = lagopus_dstring_str_get(&save_dstr, &save_str)) ==
              LAGOPUS_RESULT_OK) {
            FILE *fd = NULL;
            errno = 0;

            fd = fopen(file, "w");
            if (fd != NULL) {
              if (fputs((IS_VALID_STRING(save_str) == true) ? save_str : "",
                        fd) != EOF) {
                ret = LAGOPUS_RESULT_OK;
              } else {
                ret = LAGOPUS_RESULT_OUTPUT_FAILURE;
              }
              (void)fclose(fd);
            } else {
              ret = LAGOPUS_RESULT_POSIX_API_ERROR;
            }
          }
          free((void *)save_str);
          (void)lagopus_dstring_clear(&save_dstr);
          (void)lagopus_dstring_destroy(&save_dstr);
        }
      } else {
        errno = err;
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_NOT_ALLOWED;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (result != NULL) {
    (void)lagopus_dstring_clear(result);
    (void)datastore_json_result_setf(result, ret, "\"\"");
  }

  return ret;
}


lagopus_result_t
datastore_interp_destroy_obj(datastore_interp_t *iptr,
                             const char *namespace,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true) {
    ret = s_destroy_obj(iptr, namespace, result);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_atomic_begin(datastore_interp_t *iptr,
                              const char file[],
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true) {
    ret = s_atomic_begin(iptr, file, result);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_atomic_abort(datastore_interp_t *iptr,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true) {
    ret = s_atomic_abort(iptr, result);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_atomic_commit(datastore_interp_t *iptr,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true) {
    ret = s_atomic_commit(iptr, result);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_atomic_rollback(datastore_interp_t *iptr,
                                 lagopus_dstring_t *result,
                                 bool force) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true) {
    ret = s_atomic_rollback(iptr, result, force);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
datastore_interp_dryrun_begin(datastore_interp_t *iptr,
                              const char *cur_namespace,
                              const char *dup_namespace,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true) {
    ret = s_dryrun_begin(iptr);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = s_duplicate_obj(iptr, cur_namespace, dup_namespace, result);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        ret = s_destroy_obj(iptr, dup_namespace, result);
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
datastore_interp_dryrun_end(datastore_interp_t *iptr,
                            const char *namespace,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL &&
      s_is_interp_locked(*iptr) == true &&
      s_is_interp_functional(*iptr) == true) {
    ret = s_destroy_obj(iptr, namespace, result);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = s_dryrun_end(iptr);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
      }
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





lagopus_result_t
datastore_interp_get_current_configurater(const datastore_interp_t *iptr,
    const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL && namep != NULL) {
    *namep = (const char *)((*iptr)->m_current_configurator);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_get_current_file_context(const datastore_interp_t *iptr,
    const char **filep,
    size_t *linep,
    void **stream_inp,
    void **stream_outp,
    datastore_gets_proc_t *gets_procp,
    datastore_printf_proc_t
    *printf_procp,
    datastore_config_type_t *typep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && *iptr != NULL) {
    if (filep != NULL) {
      *filep = NULL;
    }
    if (linep != NULL) {
      *linep = 0;
    }

    if (stream_inp != NULL) {
      *stream_inp = (*iptr)->m_lr_ctx.m_stream_in;
    }
    if (stream_outp != NULL) {
      *stream_outp = (*iptr)->m_lr_ctx.m_stream_out;
    }

    if (gets_procp != NULL) {
      *gets_procp = (*iptr)->m_lr_ctx.m_gets_proc;
    }
    if (printf_procp != NULL) {
      *printf_procp = (*iptr)->m_lr_ctx.m_printf_proc;
    }

    if (typep != NULL) {
      *typep = DATASTORE_CONFIG_TYPE_UNKNOWN;
    }

    if (s_is_interp_interactive(*iptr) == false) {
      if (typep != NULL) {
        *typep = DATASTORE_CONFIG_TYPE_FILE;
      }
      if (filep != NULL) {
        *filep = (*iptr)->m_lr_ctx.m_filename;
      }
      if (linep != NULL) {
        *linep = (*iptr)->m_lr_ctx.m_lineno;
      }
    } else {
      if ((*iptr)->m_lr_ctx.m_gets_proc == s_session_gets &&
          (*iptr)->m_lr_ctx.m_printf_proc == s_session_printf) {
        if (typep != NULL) {
          *typep = DATASTORE_CONFIG_TYPE_STREAM_SESSION;
        }
      } else if ((*iptr)->m_lr_ctx.m_gets_proc == s_FILE_gets &&
                 (*iptr)->m_lr_ctx.m_printf_proc == s_FILE_printf) {
        if (typep != NULL) {
          *typep = DATASTORE_CONFIG_TYPE_STREAM_FD;
        }
      }
    }

    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
datastore_interp_eval_cmd(datastore_interp_t *iptr,
                          const char *cmd,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *stream_in = NULL;
  datastore_gets_proc_t getsproc;

  if (iptr != NULL && *iptr != NULL && cmd != NULL) {
    ret = datastore_interp_get_current_file_context(iptr,
                                                    NULL,
                                                    NULL,
                                                    &stream_in,
                                                    NULL,
                                                    &getsproc,
                                                    NULL,
                                                    NULL);
    if (ret == LAGOPUS_RESULT_OK) {
      (*iptr)->m_cmd_string = strdup(cmd);

      if ((*iptr)->m_cmd_string != NULL) {
        ret = s_eval_stream_wrap(iptr,
                                 (const char *) (*iptr)->m_current_configurator,
                                 (*iptr)->m_is_stream,
                                 stream_in, getsproc,
                                 NULL, NULL,
                                 result);
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
      free((*iptr)->m_cmd_string);
      (*iptr)->m_cmd_string = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


bool
datastore_interp_is_eval_cmd(datastore_interp_t *iptr) {
  bool b = false;

  if (iptr != NULL && *iptr != NULL &&
      (*iptr)->m_cmd_string != NULL) {
    b = true;
  }

  return b;
}




lagopus_result_t
datastore_interp_blocking_session_set(datastore_interp_t *iptr,
                                      struct session *session,
                                      lagopus_thread_t *thd) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  session_info_t *si;
  uint64_t session_id = 0LL;

  if (iptr != NULL && *iptr != NULL &&
      session != NULL && thd != NULL) {
    ret = s_session_info_create(&si, session, thd);
    if (ret == LAGOPUS_RESULT_OK) {
      session_id = session_id_get(session);
      if ((ret = lagopus_hashmap_add(&((*iptr)->m_blocking_sessions),
                                     (void *) session_id,
                                     (void **) &si,
                                     false)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
      }
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
datastore_interp_blocking_session_unset(datastore_interp_t *iptr,
                                        struct session *session) {
  uint64_t session_id = 0LL;
  session_info_t *si;

  if (iptr != NULL && *iptr != NULL && session != NULL) {
    session_id = session_id_get(session);
    lagopus_hashmap_delete(&((*iptr)->m_blocking_sessions),
                           (void *) session_id,
                           (void **) &si, true);
  }
}
