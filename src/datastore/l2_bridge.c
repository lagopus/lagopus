/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "ns_util.h"

typedef struct l2_bridge_attr {
  uint64_t expire;
  uint64_t max_entries;
  char tmp_dir[PATH_MAX];
} l2_bridge_attr_t;

typedef struct l2_bridge_conf {
  const char *name;
  l2_bridge_attr_t *current_attr;
  l2_bridge_attr_t *modified_attr;
  bool is_used;
  bool is_enabled;
  bool is_destroying;
  bool is_enabling;
  bool is_disabling;
  char bridge_name[DATASTORE_BRIDGE_FULLNAME_MAX + 1];
} l2_bridge_conf_t;

static lagopus_hashmap_t l2_bridge_table = NULL;

static inline void
l2_bridge_attr_destroy(l2_bridge_attr_t *attr) {
  if (attr != NULL) {
    free((void *) attr);
  }
}

static inline lagopus_result_t
l2_bridge_attr_create(l2_bridge_attr_t **attr) {
  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    l2_bridge_attr_destroy(*attr);
    *attr = NULL;
  }

  if ((*attr = (l2_bridge_attr_t *) malloc(sizeof(l2_bridge_attr_t)))
      == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  (*attr)->expire = 300;
  (*attr)->max_entries = 1000000;
  strncpy((*attr)->tmp_dir, DATASTORE_TMP_DIR, PATH_MAX - 1);

  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
l2_bridge_attr_duplicate(const l2_bridge_attr_t *src_attr,
                         l2_bridge_attr_t **dst_attr, const char *namespace) {
  lagopus_result_t rc;
  size_t len = 0;

  (void)namespace;

  if (src_attr == NULL || dst_attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_attr != NULL) {
    l2_bridge_attr_destroy(*dst_attr);
    *dst_attr = NULL;
  }

  rc = l2_bridge_attr_create(dst_attr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  (*dst_attr)->expire = src_attr->expire;
  (*dst_attr)->max_entries = src_attr->max_entries;

  if ((len = strlen(src_attr->tmp_dir))
      <= PATH_MAX - 1) {
    strncpy((*dst_attr)->tmp_dir, src_attr->tmp_dir, len);
    (*dst_attr)->tmp_dir[len] = '\0';
    return LAGOPUS_RESULT_OK;
  } else {
    rc = LAGOPUS_RESULT_TOO_LONG;
    goto error;
  }

error:
  l2_bridge_attr_destroy(*dst_attr);
  *dst_attr = NULL;
  return rc;
}

static inline bool
l2_bridge_attr_equals(l2_bridge_attr_t *attr0, l2_bridge_attr_t *attr1) {
  if (attr0 == NULL && attr1 == NULL) {
    return true;
  } else if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((attr0->expire == attr1->expire) &&
      (attr0->max_entries == attr1->max_entries) &&
      (strcmp(attr0->tmp_dir, attr1->tmp_dir) == 0)) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
l2_bridge_conf_create(l2_bridge_conf_t **conf, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_attr_t *default_modified_attr = NULL;

  if (conf != NULL && IS_VALID_STRING(name) == true) {
    char *cname = strdup(name);
    if (IS_VALID_STRING(cname) == true) {
      *conf = (l2_bridge_conf_t *) malloc(sizeof(l2_bridge_conf_t));
      ret = l2_bridge_attr_create(&default_modified_attr);
      if (*conf != NULL && ret == LAGOPUS_RESULT_OK) {
        (*conf)->name = cname;
        (*conf)->current_attr = NULL;
        (*conf)->modified_attr = default_modified_attr;
        (*conf)->is_used = false;
        (*conf)->is_enabled = false;
        (*conf)->is_destroying = false;
        (*conf)->is_enabling = false;
        (*conf)->is_disabling = false;
        (*conf)->bridge_name[0] = '\0';

        return LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto error;
      }
    }
  error:
    free((void *) default_modified_attr);
    free((void *) *conf);
    *conf = NULL;
    free((void *) cname);
    return ret;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline void
l2_bridge_conf_destroy(l2_bridge_conf_t *conf) {
  if (conf != NULL) {
    free((void *) (conf->name));
    if (conf->current_attr != NULL) {
      l2_bridge_attr_destroy(conf->current_attr);
    }
    if (conf->modified_attr != NULL) {
      l2_bridge_attr_destroy(conf->modified_attr);
    }
  }
  free((void *) conf);
}

static inline lagopus_result_t
l2_bridge_conf_duplicate(const l2_bridge_conf_t *src_conf,
                         l2_bridge_conf_t **dst_conf, const char *namespace) {
  lagopus_result_t rc;
  l2_bridge_attr_t *dst_current_attr = NULL;
  l2_bridge_attr_t *dst_modified_attr = NULL;
  size_t len = 0;
  char *buf = NULL;

  if (src_conf == NULL || dst_conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_conf != NULL) {
    l2_bridge_conf_destroy(*dst_conf);
    *dst_conf = NULL;
  }

  if (namespace == NULL) {
    rc = l2_bridge_conf_create(dst_conf, src_conf->name);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  } else {
    if ((len = strlen(src_conf->name)) <= DATASTORE_L2_BRIDGE_FULLNAME_MAX) {
      buf = (char *) malloc(sizeof(char) * (len + 1));

      rc = ns_replace_namespace(src_conf->name, namespace, &buf);
      if (rc == LAGOPUS_RESULT_OK) {
        rc = l2_bridge_conf_create(dst_conf, buf);
        if (rc != LAGOPUS_RESULT_OK) {
          goto error;
        }
      } else {
        goto error;
      }
      free(buf);
    } else {
      rc = LAGOPUS_RESULT_TOO_LONG;
      goto error;
    }
  }

  if (src_conf->current_attr != NULL) {
    rc = l2_bridge_attr_duplicate(src_conf->current_attr,
                                  &dst_current_attr, namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->current_attr = dst_current_attr;

  if (src_conf->modified_attr != NULL) {
    rc = l2_bridge_attr_duplicate(src_conf->modified_attr,
                                  &dst_modified_attr, namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->modified_attr = dst_modified_attr;

  (*dst_conf)->is_used = src_conf->is_used;
  (*dst_conf)->is_enabled = src_conf->is_enabled;
  (*dst_conf)->is_destroying = src_conf->is_destroying;
  (*dst_conf)->is_enabling = src_conf->is_enabling;
  (*dst_conf)->is_disabling = src_conf->is_disabling;

  return LAGOPUS_RESULT_OK;

error:
  free(buf);
  l2_bridge_conf_destroy(*dst_conf);
  *dst_conf = NULL;
  return rc;
}

static inline bool
l2_bridge_conf_equals(const l2_bridge_conf_t *conf0,
                      const l2_bridge_conf_t *conf1) {
  if (conf0 == NULL && conf1 == NULL) {
    return true;
  } else if (conf0 == NULL || conf1 == NULL) {
    return false;
  }

  if ((l2_bridge_attr_equals(conf0->current_attr,
                             conf1->current_attr) == true) &&
      (l2_bridge_attr_equals(conf0->modified_attr,
                             conf1->modified_attr) == true) &&
      (conf0->is_used == conf1->is_used) &&
      (conf0->is_enabled == conf1->is_enabled) &&
      (conf0->is_destroying == conf1->is_destroying) &&
      (conf0->is_enabling == conf1->is_enabling) &&
      (conf0->is_disabling == conf1->is_disabling)) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
l2_bridge_conf_add(l2_bridge_conf_t *conf) {
  if (l2_bridge_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      void *val = (void *) conf;
      return lagopus_hashmap_add(&l2_bridge_table,
                                 (char *) (conf->name),
                                 &val, false);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

static inline lagopus_result_t
l2_bridge_conf_delete(l2_bridge_conf_t *conf) {
  if (l2_bridge_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      return lagopus_hashmap_delete(&l2_bridge_table, (void *) conf->name,
                                    NULL, true);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

typedef struct {
  l2_bridge_conf_t **m_configs;
  size_t m_n_configs;
  size_t m_max;
  const char *m_namespace;
} l2_bridge_conf_iter_ctx_t;

static bool
l2_bridge_conf_iterate(void *key, void *val, lagopus_hashentry_t he,
                  void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;
  char *prefix = NULL;
  size_t len = 0;
  bool result = false;

  char *fullname = (char *)key;
  l2_bridge_conf_iter_ctx_t *ctx =
    (l2_bridge_conf_iter_ctx_t *)arg;

  (void)he;
  if (((l2_bridge_conf_t *) val)->is_destroying == false) {
    if (ctx->m_namespace == NULL) {
      if (ctx->m_n_configs < ctx->m_max) {
        ctx->m_configs[ctx->m_n_configs++] =
          (l2_bridge_conf_t *)val;
        result = true;
      } else {
        result = false;
      }
    } else {
      ret = lagopus_dstring_create(&ds);
      if (ret == LAGOPUS_RESULT_OK) {
        if (ctx->m_namespace[0] == '\0') {
          ret = lagopus_dstring_appendf(&ds,
                                        "%s",
                                        DATASTORE_NAMESPACE_DELIMITER);
        } else {
          ret = lagopus_dstring_appendf(&ds,
                                        "%s%s",
                                        ctx->m_namespace,
                                        DATASTORE_NAMESPACE_DELIMITER);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_str_get(&ds, &prefix);
          if (ret == LAGOPUS_RESULT_OK) {
            len = strlen(prefix);
            if (ctx->m_n_configs < ctx->m_max &&
                strncmp(fullname, prefix, len) == 0) {
              ctx->m_configs[ctx->m_n_configs++] =
                (l2_bridge_conf_t *)val;
            }
            result = true;
          } else {
            lagopus_msg_warning("dstring get failed.\n");
            result = false;
          }
        } else {
          lagopus_msg_warning("dstring append failed.\n");
          result = false;
        }
      } else {
        lagopus_msg_warning("dstring create failed.\n");
        result = false;
      }

      free((void *) prefix);

      (void)lagopus_dstring_clear(&ds);
      (void)lagopus_dstring_destroy(&ds);
    }
  } else {
    /* skip destroying conf.*/
    result = true;
  }

  return result;
}

static inline lagopus_result_t
l2_bridge_conf_list(l2_bridge_conf_t ***list, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (l2_bridge_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  }

  if (list != NULL) {
    size_t n = (size_t) lagopus_hashmap_size(&l2_bridge_table);
    *list = NULL;
    if (n > 0) {
      l2_bridge_conf_t **configs =
        (l2_bridge_conf_t **) malloc(sizeof(l2_bridge_conf_t *) * n);
      if (configs != NULL) {
        l2_bridge_conf_iter_ctx_t ctx;
        ctx.m_configs = configs;
        ctx.m_n_configs = 0;
        ctx.m_max = n;
        if (namespace == NULL) {
          ctx.m_namespace = NULL;
        } else if (namespace[0] == '\0') {
          ctx.m_namespace = "";
        } else {
          ctx.m_namespace = namespace;
        }

        ret = lagopus_hashmap_iterate(&l2_bridge_table, l2_bridge_conf_iterate,
                                      (void *) &ctx);
        if (ret == LAGOPUS_RESULT_OK) {
          *list = configs;
          ret = (ssize_t) ctx.m_n_configs;
        } else {
          free((void *) configs);
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

static inline lagopus_result_t
l2_bridge_conf_one_list(l2_bridge_conf_t ***list,
                   l2_bridge_conf_t *config) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (list != NULL && config != NULL) {
    l2_bridge_conf_t **configs = (l2_bridge_conf_t **)
        malloc(sizeof(l2_bridge_conf_t *));
    if (configs != NULL) {
      configs[0] = config;
      *list = configs;
      ret = 1;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
l2_bridge_find(const char *name, l2_bridge_conf_t **conf) {
  if (l2_bridge_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  } else if (conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (IS_VALID_STRING(name) == true && conf != NULL) {
    return lagopus_hashmap_find(&l2_bridge_table,
                                (void *)name, (void **)conf);
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

static inline lagopus_result_t
l2_bridge_get_attr(const char *name, bool current, l2_bridge_attr_t **attr) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;

  if (name == NULL || attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    l2_bridge_attr_destroy(*attr);
    *attr = NULL;
  }

  rc = l2_bridge_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK) {
    if (current == true) {
      *attr = conf->current_attr;
    } else {
      *attr = conf->modified_attr;
    }

    if (*attr == NULL) {
      rc = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  }
  return rc;
}

static inline lagopus_result_t
l2_bridge_get_expire(const l2_bridge_attr_t *attr, uint64_t *expire) {
  if (attr != NULL && expire != NULL) {
    *expire = attr->expire;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
l2_bridge_get_max_entries(const l2_bridge_attr_t *attr,
                          uint64_t *max_entries) {
  if (attr != NULL && max_entries != NULL) {
    *max_entries = attr->max_entries;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
l2_bridge_get_tmp_dir(const l2_bridge_attr_t *attr, char **tmp_dir) {
  if (attr != NULL && tmp_dir != NULL && *tmp_dir == NULL) {
    *tmp_dir = strdup(attr->tmp_dir);
    if (*tmp_dir == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
l2_bridge_set_expire(l2_bridge_attr_t *attr, const uint64_t expire) {
  if (attr != NULL) {
    attr->expire = expire;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
l2_bridge_set_max_entries(l2_bridge_attr_t *attr,
                          const uint64_t max_entries) {
  if (attr != NULL) {
    attr->max_entries = max_entries;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
l2_bridge_set_tmp_dir(l2_bridge_attr_t *attr,
                      const char *tmp_dir) {
  if (attr != NULL && tmp_dir != NULL) {
    size_t len = strlen(tmp_dir);
    if (len <= PATH_MAX - 1) {
      strncpy(attr->tmp_dir, tmp_dir, len);
      attr->tmp_dir[len] = '\0';
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static void
l2_bridge_conf_freeup(void *conf) {
  l2_bridge_conf_destroy((l2_bridge_conf_t *) conf);
}

static void
l2_bridge_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&l2_bridge_table);
}

static lagopus_result_t
l2_bridge_initialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  if ((r = lagopus_hashmap_create(&l2_bridge_table, LAGOPUS_HASHMAP_TYPE_STRING,
                                  l2_bridge_conf_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }

  (void)pthread_atfork(NULL, NULL, l2_bridge_child_at_fork);

done:
  return r;

}

static void
l2_bridge_finalize(void) {
  lagopus_hashmap_destroy(&l2_bridge_table, true);
}

//
// internal datastore
//

bool
l2_bridge_exists(const char *name) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;

  rc = l2_bridge_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK && conf != NULL) {
    return true;
  }
  return false;
}

lagopus_result_t
l2_bridge_set_used(const char *name, bool is_used) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = l2_bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_used = is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
l2_bridge_set_enabled(const char *name, bool is_enabled) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = l2_bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_enabled = is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

//
// public
//

lagopus_result_t
datastore_l2_bridge_is_enabled(const char *name, bool *is_enabled) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_enabled != NULL) {
    rc = l2_bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_enabled = conf->is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return rc;
}

lagopus_result_t
datastore_l2_bridge_is_used(const char *name, bool *is_used) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_used != NULL) {
    rc = l2_bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_used = conf->is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_l2_bridge_get_bridge_name(const char *name,
                                    char **bridge_name) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && bridge_name != NULL) {
    rc = l2_bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *bridge_name = strdup(conf->bridge_name);
      if (*bridge_name == NULL) {
        rc = LAGOPUS_RESULT_NO_MEMORY;
      }
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_l2_bridge_get_expire(const char *name, bool current,
                               uint64_t *expire) {
  lagopus_result_t rc;
  l2_bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && expire != NULL) {
    rc = l2_bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = l2_bridge_get_expire(attr, expire);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_l2_bridge_get_max_entries(const char *name, bool current,
                                    uint64_t *max_entries) {
  lagopus_result_t rc;
  l2_bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && max_entries != NULL) {
    rc = l2_bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = l2_bridge_get_max_entries(attr, max_entries);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_l2_bridge_get_tmp_dir(const char *name, bool current,
                                char **tmp_dir) {
  lagopus_result_t rc;
  l2_bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && tmp_dir != NULL) {
    rc = l2_bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = l2_bridge_get_tmp_dir(attr, tmp_dir);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}
