#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "conv_json.h"





#define CTOR_IDX	LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 2

#define MY_CONF_NAME		"test-conf"

#define MY_COMMAND_NAME		"testobj"





typedef void *testobj_t;


typedef struct {
  uint32_t m_intval;
  char *m_strval;
} testobj_attr_t;


typedef struct {
  testobj_t m_obj;

  const char *m_name;
  testobj_attr_t *m_current_attr;
  testobj_attr_t *m_modified_attr;
  bool m_is_enabled;
} testobj_config_t;





static lagopus_hashmap_t s_tbl;
static datastore_interp_t s_interp;

static pthread_once_t s_once = PTHREAD_ONCE_INIT;


static void	s_testobj_attr_destroy(testobj_attr_t *o);

static void	s_ctors(void) __attr_constructor__(CTOR_IDX);
static void	s_dtors(void) __attr_destructor__(CTOR_IDX);





/*
 * Dummy object constructor/destructor
 */
static inline testobj_t
s_testobj_create(uint32_t intval, char *strval) {
  (void)intval;
  (void)strval;

  return (testobj_t)malloc(sizeof(uint32_t));
}


static inline lagopus_result_t
s_testobj_start(testobj_t o) {
  (void)o;
  return LAGOPUS_RESULT_OK;
}


static inline lagopus_result_t
s_testobj_shutdown(testobj_t o, shutdown_grace_level_t l) {
  (void)l;
  (void)o;
  return LAGOPUS_RESULT_OK;
}


static inline void
s_testobj_destroy(testobj_t o) {
  (void)s_testobj_shutdown(o, SHUTDOWN_RIGHT_NOW);
  free((void *)o);
}





static inline testobj_config_t *
s_config_create(const char *name) {
  testobj_config_t *ret = NULL;

  if (IS_VALID_STRING(name) == true) {
    char *cname = strdup(name);
    if (IS_VALID_STRING(cname) == true) {
      ret = (testobj_config_t *)malloc(sizeof(testobj_config_t));
      if (ret != NULL) {
        (void)memset((void *)ret, 0, sizeof(testobj_config_t));
        ret->m_name = cname;
      } else {
        free((void *)cname);
      }
    }
  }

  return ret;
}


static inline void
s_config_destroy(testobj_config_t *o) {
  if (o != NULL) {
    if (o->m_obj != NULL) {
      s_testobj_destroy(o->m_obj);
    }
    free((void *)(o->m_name));
    if (o->m_current_attr != NULL) {
      s_testobj_attr_destroy(o->m_current_attr);
    }
    if (o->m_modified_attr != NULL) {
      s_testobj_attr_destroy(o->m_modified_attr);
    }
  }
  free((void *)o);
}


static inline lagopus_result_t
s_config_add(testobj_config_t *o) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (o != NULL && IS_VALID_STRING(o->m_name) == true) {
    void *val = (void *)o;
    ret = lagopus_hashmap_add(&s_tbl, (void *)(o->m_name), &val, false);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_config_delete(testobj_config_t *o) {
  if (o != NULL && IS_VALID_STRING(o->m_name) == true) {
    (void)lagopus_hashmap_delete(&s_tbl, (void *)(o->m_name), NULL, true);
  }
}


static inline testobj_config_t *
s_config_find(const char *name) {
  testobj_config_t *ret = NULL;

  if (IS_VALID_STRING(name) == true) {
    void *val;
    if (lagopus_hashmap_find(&s_tbl, (void *)name, &val) ==
        LAGOPUS_RESULT_OK) {
      ret = (testobj_config_t *)val;
    }
  }

  return ret;
}


typedef struct {
  testobj_config_t **m_configs;
  size_t m_n_configs;
  size_t m_max;
} testobj_config_iter_ctx_t;


static bool
s_config_iterate(void *key, void *val, lagopus_hashentry_t he,
                 void *arg) {
  testobj_config_iter_ctx_t *ctx = (testobj_config_iter_ctx_t *)arg;

  (void)key;
  (void)he;

  if (ctx->m_n_configs < ctx->m_max) {
    ctx->m_configs[ctx->m_n_configs++] = (testobj_config_t *)val;
    return true;
  } else {
    return false;
  }
}


static inline lagopus_result_t
s_config_list(testobj_config_t ***list) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (list != NULL) {
    size_t n = (size_t)lagopus_hashmap_size(&s_tbl);
    *list = NULL;
    if (n > 0) {
      testobj_config_t **configs = (testobj_config_t **)
                                   malloc(sizeof(testobj_config_t *) * n);
      if (configs != NULL) {
        testobj_config_iter_ctx_t ctx;
        ctx.m_configs = configs;
        ctx.m_n_configs = 0;
        ctx.m_max = n;
        ret = lagopus_hashmap_iterate(&s_tbl, s_config_iterate,
                                      (void *)&ctx);
        if (ret == LAGOPUS_RESULT_OK) {
          *list = configs;
          ret = (ssize_t)n;
        } else {
          free((void *)configs);
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





/*
 * Bean thingies.
 */


static inline testobj_attr_t *
s_testobj_attr_alloc(void) {
  testobj_attr_t *ret = (testobj_attr_t *)malloc(sizeof(testobj_attr_t));
  if (ret != NULL) {
    (void)memset((void *)ret, 0, sizeof(testobj_attr_t));
  }
  return ret;
}


static inline void
s_testobj_attr_destroy(testobj_attr_t *o) {
  if (o != NULL) {
    if (o->m_strval != NULL) {
      free((void *)o->m_strval);
    }
    free((void *)o);
  }
}


/*
 * A deep copier.
 */
static inline testobj_attr_t *
s_testobj_attr_duplicate(testobj_attr_t *o) {
  testobj_attr_t *ret = NULL;
  char *name = NULL;

  if (o != NULL) {
    name = strdup(o->m_strval);
    ret = s_testobj_attr_alloc();
    if (ret != NULL && IS_VALID_STRING(name) == true) {
      ret->m_intval = o->m_intval;
      ret->m_strval = name;
    } else {
      free((void *)ret);
      free((void *)name);
    }
  }

  return ret;
}


/*
 * equals
 */
static inline bool
s_testobj_attr_equals(testobj_attr_t *o0, testobj_attr_t *o1) {
  bool ret = false;

  if (o0 != NULL && o1 != NULL &&
      o0->m_intval == o1->m_intval &&
      strcmp(o0->m_strval, o1->m_strval) == 0) {
    ret = true;
  }

  return ret;
}


static inline lagopus_result_t
s_attr_get_intval(testobj_attr_t *o, uint32_t *valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (o != NULL && valptr != NULL) {
    *valptr = o->m_intval;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_attr_set_intval(testobj_attr_t *o, uint32_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (o != NULL) {
    o->m_intval = val;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_attr_get_strval(testobj_attr_t *o, char **valptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (o != NULL && valptr != NULL) {
    *valptr = o->m_strval;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_attr_set_strval(testobj_attr_t *o, const char *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (o != NULL && IS_VALID_STRING(val) == true) {
    char *c = strdup(val);
    if (IS_VALID_STRING(c) == true) {
      if (o->m_strval != NULL) {
        free((void *)(o->m_strval));
      }
      o->m_strval = c;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_attr_dump(lagopus_dstring_t *ds, testobj_attr_t *o) {
  if (o != NULL) {
    (void)lagopus_dstring_appendf(ds,
                                  "{\"intval\": %u, \"strval\": \"%s\"}",
                                  o->m_intval,
                                  (IS_VALID_STRING(o->m_strval) == true) ?
                                  o->m_strval : "");
  }
}





static lagopus_result_t
s_update(datastore_interp_t *iptr,
         datastore_interp_state_t state,
         void *obj,
         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)result;

  if (iptr != NULL && *iptr != NULL && obj != NULL) {
    testobj_config_t *o = (testobj_config_t *)obj;

    switch (state) {
      case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
        bool is_created = false;

        if (o->m_obj == NULL ||
            (o->m_modified_attr != NULL &&
             s_testobj_attr_equals(o->m_current_attr,
                                   o->m_modified_attr) == false)) {
          /*
           * (re-)create the obj.
           */

          /*
           * Destroy the current obj if needed.
           */
          if (o->m_obj != NULL) {
            s_testobj_destroy(o->m_obj);
            o->m_obj = NULL;
          }

          if (o->m_modified_attr != NULL) {
            o->m_obj = s_testobj_create(o->m_modified_attr->m_intval,
                                        o->m_modified_attr->m_strval);
            if (o->m_obj != NULL) {
              /*
               * Created successfully.
               */
              lagopus_msg_debug(1, "obj \"%s\": created.\n",
                                o->m_name);
              is_created = true;
              ret = LAGOPUS_RESULT_OK;
            } else {
              ret = LAGOPUS_RESULT_NO_MEMORY;
            }
          } else {
            ret = LAGOPUS_RESULT_INVALID_ARGS;
          }
        } else {
          /*
           * no need to re-create.
           */
          ret = LAGOPUS_RESULT_OK;
        }

        if (ret == LAGOPUS_RESULT_OK) {
          /*
           * Then start/shutdown if needed.
           */
          if (is_created == true) {
            /*
             * Update attrs.
             */
            if (o->m_current_attr != NULL) {
              s_testobj_attr_destroy(o->m_current_attr);
            }
            o->m_current_attr = o->m_modified_attr;
            o->m_modified_attr = NULL;

            if (o->m_is_enabled == true) {
              /*
               * start the obj if it WAS started.
               */
              ret = s_testobj_start(o->m_obj);
              if (ret == LAGOPUS_RESULT_OK) {
                lagopus_msg_debug(1, "obj \"%s\": created and enabled.\n",
                                  o->m_name);
              } else {
                lagopus_perror(ret);
              }
            }
          } else {
            if (o->m_is_enabled == true) {
              ret = s_testobj_start(o->m_obj);
              if (ret == LAGOPUS_RESULT_OK) {
                lagopus_msg_debug(1, "obj \"%s\": enabled.\n",
                                  o->m_name);
              } else {
                lagopus_perror(ret);
              }
            } else {
              ret = s_testobj_shutdown(o->m_obj, SHUTDOWN_RIGHT_NOW);
              if (ret == LAGOPUS_RESULT_OK) {
                lagopus_msg_debug(1, "obj \"%s\": disabled.\n",
                                  o->m_name);
              } else {
                lagopus_perror(ret);
              }
            }
          }
        }

        break;
      }

      default: {
        break;
      }

    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static lagopus_result_t
s_enable(datastore_interp_t *iptr,
         datastore_interp_state_t state,
         void *obj,
         bool *currentp,
         bool enabled,
         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)iptr;
  (void)state;
  (void)obj;
  (void)currentp;
  (void)enabled;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_serialize(datastore_interp_t *iptr,
            datastore_interp_state_t state,
            const void *obj,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)iptr;
  (void)state;
  (void)obj;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_destroy(datastore_interp_t *iptr,
          datastore_interp_state_t state,
          void *obj,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)iptr;
  (void)state;
  (void)obj;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_compare(const void *obj0, const void *obj1, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj0 != NULL && obj1 != NULL && result != NULL) {
    *result = strcmp(((testobj_config_t *)obj0)->m_name,
                     ((testobj_config_t *)obj1)->m_name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static lagopus_result_t
s_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((testobj_config_t *)obj)->m_name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static lagopus_result_t
s_duplicate(const void *obj, const char *namespace) {
  (void)obj;
  (void)namespace;
  return LAGOPUS_RESULT_OK;
}


static lagopus_result_t
s_parse(datastore_interp_t *iptr,
        datastore_interp_state_t state,
        size_t argc, const char *const argv[],
        lagopus_hashmap_t *hptr,
        datastore_update_proc_t u_proc,
        datastore_enable_proc_t e_proc,
        datastore_serialize_proc_t s_proc,
        datastore_destroy_proc_t d_proc,
        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  char *objname = NULL;
  testobj_config_t *o = NULL;
  const char *cname = NULL;
  datastore_config_type_t ftype = DATASTORE_CONFIG_TYPE_UNKNOWN;
  const char *filename = NULL;
  size_t lineno = 0;
  datastore_printf_proc_t printf_proc = NULL;
  void *stream_out = NULL;

  (void)hptr;
  (void)e_proc;
  (void)s_proc;
  (void)d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  if ((ret = datastore_interp_get_current_configurater(iptr, &cname)) ==
      LAGOPUS_RESULT_OK &&
      IS_VALID_STRING(cname) == true) {
    fprintf(stderr, "The current confiurater is '%s'\n", cname);
  } else {
    lagopus_perror(ret);
  }

  if ((ret = datastore_interp_get_current_file_context(iptr,
             &filename, &lineno,
             NULL, &stream_out,
             NULL, &printf_proc,
             &ftype)) ==
      LAGOPUS_RESULT_OK &&
      stream_out != NULL &&
      printf_proc != NULL &&
      ftype != DATASTORE_CONFIG_TYPE_UNKNOWN) {
    if (ftype == DATASTORE_CONFIG_TYPE_FILE) {
      printf_proc(stream_out, "debug: file '%s', line "PFSZS(4, u)
                  ": test.\n",
                  filename, lineno);
    } else if (ftype == DATASTORE_CONFIG_TYPE_STREAM_FD ||
               ftype == DATASTORE_CONFIG_TYPE_STREAM_SESSION) {
      printf_proc(stream_out, "debug: test.\n");
    }
  } else {
    lagopus_perror(ret);
  }

  argv++;
  if (IS_VALID_STRING(*argv) == true) {
    if ((ret = lagopus_str_unescape(*argv, "\"'", &objname)) < 0) {
      goto done;
    } else {
      ret = LAGOPUS_RESULT_ANY_FAILURES;
    }
  }

  if (IS_VALID_STRING(objname) == false) {

    /*
     * list all objs.
     */
    testobj_config_t **l;
    lagopus_result_t n = s_config_list(&l);

    (void)lagopus_dstring_appendf(result, "[");
    for (i = 0; i < (size_t)n; i++) {
      (void)lagopus_dstring_appendf(result, "{\"name\": \"%s\", ",
                                    l[i]->m_name);
      s_attr_dump(result,
                  (l[i]->m_current_attr != NULL) ?
                  l[i]->m_current_attr : l[i]->m_modified_attr);
      (void)lagopus_dstring_appendf(result, "}");
      if (i < (size_t)(n - 1)) {
        (void)lagopus_dstring_appendf(result, ", ");
      }
    }
    (void)lagopus_dstring_appendf(result, "]");

    free((void *)l);

    ret = LAGOPUS_RESULT_OK;

  } else if (IS_VALID_STRING(*(argv + 1)) == false) {
  dump_an_obj:
    /*
     * dump a specified object.
     */
    o = s_config_find(objname);
    if (o != NULL) {
      s_attr_dump(result,
                  (o->m_current_attr != NULL) ?
                  o->m_current_attr : o->m_modified_attr);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
    }

  } else {

    int match_create = -INT_MAX;
    uint32_t intval = 0;
    char *strval = NULL;

    o = s_config_find(objname);

    argv++;
    if (strcasecmp(*argv, "enable") == 0) {

      if (o != NULL) {
        if (u_proc != NULL) {
          if (state != DATASTORE_INTERP_STATE_ATOMIC) {
            o->m_is_enabled = true;
            ret = u_proc(iptr, state, (void *)o, result);
          }
        } else {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
        }
      } else {
        ret = LAGOPUS_RESULT_INVALID_OBJECT;
      }

    } else if (strcasecmp(*argv, "disable") == 0) {

      if (o != NULL) {
        o->m_is_enabled = false;
        ret = u_proc(iptr, state, (void *)o, result);
      } else {
        ret = LAGOPUS_RESULT_INVALID_OBJECT;
      }

    } else if ((match_create = strcasecmp(*argv, "create")) == 0 ||
               strcasecmp(*argv, "config") == 0) {

      bool is_create = (match_create == 0) ? true : false;

      if (is_create == true) {

      do_create:
        if (o == NULL) {
          o = s_config_create(objname);
          if (o == NULL) {
            ret = LAGOPUS_RESULT_NO_MEMORY;
            goto done;
          }
          o->m_current_attr = NULL;
          o->m_modified_attr = s_testobj_attr_alloc();
          if (o->m_modified_attr == NULL) {
            s_config_destroy(o);
            ret = LAGOPUS_RESULT_NO_MEMORY;
            goto done;
          }
        } else {
          ret = LAGOPUS_RESULT_ALREADY_EXISTS;
          goto done;
        }

      } else {

        if (o == NULL) {
          /*
           * treat this case as "create".
           */
          is_create = true;
          goto do_create;
        } else {
          if (o->m_modified_attr == NULL) {
            if (o->m_current_attr != NULL) {
              /*
               * already exists. copy it.
               */
              o->m_modified_attr =
                s_testobj_attr_duplicate(o->m_current_attr);
              if (o->m_modified_attr == NULL) {
                ret = LAGOPUS_RESULT_NO_MEMORY;
                goto done;
              }
            } else {
              /*
               * An object exists but both attrs are NULL. must not happens.
               */
              lagopus_exit_fatal("A config object for \"%s\" exists but no "
                                 "attribute objject exists.\n",
                                 objname);
            }
          }
        }

      }

      argv++;
      if (IS_VALID_STRING(*argv) == false) {
        /*
         * create/config without options.
         */
        if (is_create == true) {
          ret = LAGOPUS_RESULT_OK;
          goto conf_done;
        } else {
          goto dump_an_obj;
        }
      }

      while (*argv != NULL) {

        if (strcasecmp(*argv, "-intval") == 0) {

          if (IS_VALID_STRING(*(argv + 1)) == true) {
            if ((ret = lagopus_str_parse_uint32(*(++argv), &intval)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = s_attr_set_intval(o->m_modified_attr, intval)) !=
                  LAGOPUS_RESULT_OK) {
                goto done;
              }
            } else {
              goto done;
            }
          } else {
            if (is_create == false) {
              if ((ret = s_attr_get_intval(
                           (o->m_current_attr != NULL) ?
                           o->m_current_attr : o->m_modified_attr, &intval)) ==
                  LAGOPUS_RESULT_OK) {
                (void)lagopus_dstring_appendf(result,
                                              "{\"intval\": %u}", intval);
              } else {
                goto done;
              }
            } else {
              ret = LAGOPUS_RESULT_INVALID_ARGS;
              goto done;
            }
          }

        } else if (strcasecmp(*argv, "-strval") == 0) {

          if (IS_VALID_STRING(*(argv + 1)) == true) {
            if ((ret = s_attr_set_strval(o->m_modified_attr, *(++argv))) !=
                LAGOPUS_RESULT_OK) {
              goto done;
            }
          } else {
            if (is_create == false) {
              (void)s_attr_get_strval(
                (o->m_current_attr != NULL) ?
                o->m_current_attr : o->m_modified_attr, &strval);
              (void)lagopus_dstring_appendf(result,
                                            "{\"strval\": \"%s\"}",
                                            (IS_VALID_STRING(strval) == true) ?
                                            strval : "");
              ret = LAGOPUS_RESULT_OK;
              goto done;
            } else {
              ret = LAGOPUS_RESULT_INVALID_ARGS;
              goto done;
            }
          }

        } else {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
          goto done;
        }

        argv++;
      }

    conf_done:
      if (ret == LAGOPUS_RESULT_OK) {

        if (is_create == true) {
          /*
           * Add the config obj. to the DB.
           */
          ret = s_config_add(o);
          if (ret != LAGOPUS_RESULT_OK) {
            s_config_destroy(o);
            goto done;
          }
        }

        if (state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
          if (u_proc != NULL) {
            ret = u_proc(iptr, state, (void *)o, result);
          } else {
            ret = LAGOPUS_RESULT_INVALID_ARGS;
          }
        }

      }

    } else if (strcasecmp(*argv, "dump") == 0) {
      void **l = NULL;
      ret = datastore_get_objects(MY_COMMAND_NAME, &l, true);
      if (ret >= 0) {
        size_t n_objs = (size_t)ret;

        for (i = 0; i < n_objs; i++) {
          lagopus_msg_debug(1, "obj[" PFSZS(3, u) "]: '%s'\n", i,
                            ((testobj_config_t *)l[i])->m_name);
        }
        free((void *)l);

        ret = LAGOPUS_RESULT_OK;
      }
      goto done;

    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }

  }

done:
  free((void *)objname);
  return ret;
}





static void
s_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&s_tbl);
}


static void
s_config_freeup(void *o) {
  s_config_destroy((testobj_config_t *)o);
}


static void
s_once_proc(void) {
  lagopus_result_t r;

  if ((r = lagopus_hashmap_create(&s_tbl, LAGOPUS_HASHMAP_TYPE_STRING,
                                  s_config_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize an object table.\n");
  }

  if ((r = datastore_register_table(MY_COMMAND_NAME,
                                    &s_tbl,
                                    s_update,
                                    s_enable,
                                    s_serialize,
                                    s_destroy,
                                    s_compare,
                                    s_getname,
                                    s_duplicate)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register an object table for \"%s\".\n",
                       MY_COMMAND_NAME);
  } else {
    lagopus_hashmap_t s_tbl2 = NULL;
    datastore_update_proc_t u_proc = NULL;
    datastore_enable_proc_t e_proc = NULL;
    datastore_serialize_proc_t s_proc = NULL;
    datastore_destroy_proc_t d_proc = NULL;
    datastore_compare_proc_t c_proc = NULL;
    datastore_getname_proc_t n_proc = NULL;
    datastore_duplicate_proc_t dup_proc = NULL;

    if ((r = datastore_find_table(MY_COMMAND_NAME,
                                  &s_tbl2,
                                  &u_proc,
                                  &e_proc,
                                  &s_proc,
                                  &d_proc,
                                  &c_proc,
                                  &n_proc,
                                  &dup_proc)) != LAGOPUS_RESULT_OK ||
        s_tbl != s_tbl2 ||
        u_proc != s_update ||
        e_proc != s_enable ||
        s_proc != s_serialize ||
        d_proc != s_destroy ||
        c_proc != s_compare ||
        n_proc != s_getname ||
        dup_proc != s_duplicate) {
      if (r != LAGOPUS_RESULT_OK) {
        lagopus_perror(r);
      }
      lagopus_exit_fatal("Can't get the table info "
                         "that was just registered.\n");
    } else {
      lagopus_msg_debug(1,
                        "s_tbl = %p, s_tbl2 = %p, "
                        "s_update = %p, u_proc = %p\n",
                        s_tbl, s_tbl2,
                        s_update, u_proc);
    }
  }

  if ((r = datastore_create_interp(&s_interp)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't create the datastore interpretor.\n");
  }

  if ((r = datastore_interp_register_command(&s_interp, NULL,
           MY_COMMAND_NAME, s_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register an object command \"%s\".\n",
                       MY_COMMAND_NAME);
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

  lagopus_msg_debug(10, "datastore object \"%s\" initialzied.\n",
                    MY_COMMAND_NAME);
}


static inline void
s_final(void) {
  datastore_destroy_interp(&s_interp);
  lagopus_hashmap_destroy(&s_tbl, true);
}


static void
s_dtors(void) {
  s_final();

  lagopus_msg_debug(10, "datastore object \"%s\" finalized.\n",
                    MY_COMMAND_NAME);
}





int
main(int argc, const char *const argv[]) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  const char *file = argv[1];

  (void)argc;

  if ((r = datastore_register_configurator(MY_CONF_NAME)) ==
      LAGOPUS_RESULT_OK) {

    lagopus_dstring_t ds;
    char *str = NULL;

    (void)lagopus_dstring_create(&ds);
    (void)datastore_json_result_setf(&ds, LAGOPUS_RESULT_UNSUPPORTED, "\"\"");
    (void)lagopus_dstring_str_get(&ds, &str);
    fprintf(stderr, "test: '%s'\n", str);
    free((void *)str);
    str = NULL;

    if (IS_VALID_STRING(file) == true) {
      r = datastore_interp_eval_file(&s_interp, MY_CONF_NAME, file, &ds);
    } else {
      while ((r = datastore_interp_eval_fd(&s_interp, MY_CONF_NAME,
                                           stdin, stdout, false, &ds)) ==
             LAGOPUS_RESULT_OK) {
        lagopus_dstring_clear(&ds);
      }
    }

    if (r != LAGOPUS_RESULT_OK) {
      lagopus_perror(r);
    }

    (void)lagopus_dstring_str_get(&ds, &str);
    if (IS_VALID_STRING(str) == true) {
      fprintf(stdout, "%s", str);
    }

    free((void *)str);

  } else {
    lagopus_perror(r);
  }

  datastore_unregister_configurator(MY_CONF_NAME);

  return (r == LAGOPUS_RESULT_OK) ? 0 : 1;
}
