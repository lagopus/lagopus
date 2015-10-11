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

#define MINIMUM_BANDWIDTH_LIMIT 1500LL
#define MAXIMUM_BANDWIDTH_LIMIT 100000000000LL
#define MINIMUM_BURST_SIZE_LIMIT 1500LL
#define MAXIMUM_BURST_SIZE_LIMIT 100000000000LL
#define MINIMUM_BANDWIDTH_PERCENT 0
#define MAXIMUM_BANDWIDTH_PERCENT 100

#ifndef DEFAULT_BANDWIDTH_LIMIT
#define DEFAULT_BANDWIDTH_LIMIT 1500LL
#endif /* DEFAULT_BANDWIDTH_LIMIT */

#ifndef DEFAULT_BURST_SIZE_LIMIT
#define DEFAULT_BURST_SIZE_LIMIT 1500LL
#endif /* DEFAULT_BURST_SIZE_LIMIT */

#ifndef DEFAULT_BANDWIDTH_PERCENT
#define DEFAULT_BANDWIDTH_PERCENT 0
#endif /* DEFAULT_BANDWIDTH_PERCENT */

typedef struct policer_attr {
  datastore_name_info_t *action_names;
  uint64_t bandwidth_limit;
  uint64_t burst_size_limit;
  uint8_t bandwidth_percent;
} policer_attr_t;

typedef struct policer_conf {
  const char *name;
  policer_attr_t *current_attr;
  policer_attr_t *modified_attr;
  bool is_used;
  bool is_enabled;
  bool is_destroying;
  bool is_enabling;
  bool is_disabling;
} policer_conf_t;

static lagopus_hashmap_t policer_table = NULL;

static inline void
policer_attr_destroy(policer_attr_t *attr) {
  if (attr != NULL) {
    datastore_names_destroy(attr->action_names);
    free((void *) attr);
  }
}

static inline lagopus_result_t
policer_attr_create(policer_attr_t **attr) {
  lagopus_result_t rc;

  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    policer_attr_destroy(*attr);
    *attr = NULL;
  }

  if ((*attr = (policer_attr_t *) malloc(sizeof(policer_attr_t)))
      == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  (*attr)->bandwidth_limit = DEFAULT_BANDWIDTH_LIMIT;
  (*attr)->burst_size_limit = DEFAULT_BURST_SIZE_LIMIT;
  (*attr)->bandwidth_percent = DEFAULT_BANDWIDTH_PERCENT;
  (*attr)->action_names = NULL;
  rc = datastore_names_create(&((*attr)->action_names));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  return LAGOPUS_RESULT_OK;

error:
  datastore_names_destroy((*attr)->action_names);
  free((void *) *attr);
  *attr = NULL;
  return rc;
}

static inline lagopus_result_t
policer_attr_duplicate(const policer_attr_t *src_attr,
                       policer_attr_t **dst_attr, const char *namespace) {
  lagopus_result_t rc;

  if (src_attr == NULL || dst_attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_attr != NULL) {
    policer_attr_destroy(*dst_attr);
    *dst_attr = NULL;
  }

  rc = policer_attr_create(dst_attr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  (*dst_attr)->bandwidth_limit = src_attr->bandwidth_limit;
  (*dst_attr)->burst_size_limit = src_attr->burst_size_limit;
  (*dst_attr)->bandwidth_percent = src_attr->bandwidth_percent;

  rc = datastore_names_duplicate(src_attr->action_names,
                                 &((*dst_attr)->action_names), namespace);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  return LAGOPUS_RESULT_OK;

error:
  policer_attr_destroy(*dst_attr);
  *dst_attr = NULL;
  return rc;
}

static inline bool
policer_attr_action_name_exists(const policer_attr_t *attr, const char *name) {
  if (attr == NULL || name == NULL) {
    return false;
  }
  return datastore_name_exists(attr->action_names, name);
}

static inline bool
policer_attr_equals(policer_attr_t *attr0, policer_attr_t *attr1) {
  if (attr0 == NULL && attr1 == NULL) {
    return true;
  } else if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((attr0->bandwidth_limit == attr1->bandwidth_limit) &&
      (attr0->burst_size_limit == attr1->burst_size_limit) &&
      (attr0->bandwidth_percent == attr1->bandwidth_percent) &&
      (datastore_names_equals(attr0->action_names,
                              attr1->action_names) == true)) {
    return true;
  }

  return false;
}

static inline bool
policer_attr_equals_without_names(policer_attr_t *attr0,
                                  policer_attr_t *attr1) {
  if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((attr0->bandwidth_limit == attr1->bandwidth_limit) &&
      (attr0->burst_size_limit == attr1->burst_size_limit) &&
      (attr0->bandwidth_percent == attr1->bandwidth_percent)) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
policer_attr_add_action_name(const policer_attr_t *attr, const char *name) {
  if (attr == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_add_names(attr->action_names, name);
}

static inline lagopus_result_t
policer_attr_remove_action_name(const policer_attr_t *attr, const char *name) {
  if (attr == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_remove_names(attr->action_names, name);
}

static inline lagopus_result_t
policer_attr_remove_all_action_name(const policer_attr_t *attr) {
  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_remove_all_names(attr->action_names);
}

static inline lagopus_result_t
policer_conf_create(policer_conf_t **conf, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_attr_t *default_modified_attr = NULL;

  if (conf != NULL && IS_VALID_STRING(name) == true) {
    char *cname = strdup(name);
    if (IS_VALID_STRING(cname) == true) {
      *conf = (policer_conf_t *) malloc(sizeof(policer_conf_t));
      ret = policer_attr_create(&default_modified_attr);
      if (*conf != NULL && ret == LAGOPUS_RESULT_OK) {
        (*conf)->name = cname;
        (*conf)->current_attr = NULL;
        (*conf)->modified_attr = default_modified_attr;
        (*conf)->is_used = false;
        (*conf)->is_enabled = false;
        (*conf)->is_destroying = false;
        (*conf)->is_enabling = false;
        (*conf)->is_disabling = false;

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
policer_conf_destroy(policer_conf_t *conf) {
  if (conf != NULL) {
    free((void *) (conf->name));
    if (conf->current_attr != NULL) {
      policer_attr_destroy(conf->current_attr);
    }
    if (conf->modified_attr != NULL) {
      policer_attr_destroy(conf->modified_attr);
    }
  }
  free((void *) conf);
}

static inline lagopus_result_t
policer_conf_duplicate(const policer_conf_t *src_conf,
                       policer_conf_t **dst_conf,
                       const char *namespace) {
  lagopus_result_t rc;
  policer_attr_t *dst_current_attr = NULL;
  policer_attr_t *dst_modified_attr = NULL;
  size_t len = 0;
  char *buf = NULL;

  if (src_conf == NULL || dst_conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_conf != NULL) {
    policer_conf_destroy(*dst_conf);
    *dst_conf = NULL;
  }

  if (namespace == NULL) {
    rc = policer_conf_create(dst_conf, src_conf->name);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  } else {
    if ((len = strlen(src_conf->name)) <= DATASTORE_POLICER_FULLNAME_MAX) {
      rc = ns_replace_namespace(src_conf->name, namespace, &buf);
      if (rc == LAGOPUS_RESULT_OK) {
        rc = policer_conf_create(dst_conf, buf);
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
    rc = policer_attr_duplicate(src_conf->current_attr,
                                &dst_current_attr, namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->current_attr = dst_current_attr;

  if (src_conf->modified_attr != NULL) {
    rc = policer_attr_duplicate(src_conf->modified_attr,
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
  policer_conf_destroy(*dst_conf);
  *dst_conf = NULL;
  return rc;
}

static inline bool
policer_conf_equals(const policer_conf_t *conf0,
                    const policer_conf_t *conf1) {
  if (conf0 == NULL && conf1 == NULL) {
    return true;
  } else if (conf0 == NULL || conf1 == NULL) {
    return false;
  }

  if ((policer_attr_equals(conf0->current_attr,
                           conf1->current_attr) == true) &&
      (policer_attr_equals(conf0->modified_attr,
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
policer_conf_add(policer_conf_t *conf) {
  if (policer_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      void *val = (void *) conf;
      return lagopus_hashmap_add(&policer_table,
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
policer_conf_delete(policer_conf_t *conf) {
  if (policer_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      return lagopus_hashmap_delete(&policer_table, (void *) conf->name,
                                    NULL, true);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

typedef struct {
  policer_conf_t **m_configs;
  size_t m_n_configs;
  size_t m_max;
  const char *m_namespace;
} policer_conf_iter_ctx_t;

static bool
policer_conf_iterate(void *key, void *val, lagopus_hashentry_t he,
                     void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;
  char *prefix = NULL;
  size_t len = 0;
  bool result = false;

  char *fullname = (char *)key;
  policer_conf_iter_ctx_t *ctx =
    (policer_conf_iter_ctx_t *)arg;

  (void)he;
  if (((policer_conf_t *) val)->is_destroying == false) {
    if (ctx->m_namespace == NULL) {
      if (ctx->m_n_configs < ctx->m_max) {
        ctx->m_configs[ctx->m_n_configs++] =
          (policer_conf_t *)val;
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
                (policer_conf_t *)val;
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
policer_conf_list(policer_conf_t ***list, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (policer_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  }

  if (list != NULL) {
    size_t n = (size_t) lagopus_hashmap_size(&policer_table);
    *list = NULL;
    if (n > 0) {
      policer_conf_t **configs =
        (policer_conf_t **) malloc(sizeof(policer_conf_t *) * n);
      if (configs != NULL) {
        policer_conf_iter_ctx_t ctx;
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

        ret = lagopus_hashmap_iterate(&policer_table, policer_conf_iterate,
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
policer_conf_one_list(policer_conf_t ***list,
                      policer_conf_t *config) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (list != NULL && config != NULL) {
    policer_conf_t **configs = (policer_conf_t **)
                               malloc(sizeof(policer_conf_t *));
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
policer_find(const char *name, policer_conf_t **conf) {
  if (policer_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  } else if (conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (IS_VALID_STRING(name) == true && conf != NULL) {
    return lagopus_hashmap_find(&policer_table,
                                (void *)name, (void **)conf);
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

static inline lagopus_result_t
policer_get_attr(const char *name, bool current, policer_attr_t **attr) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;

  if (name == NULL || attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    policer_attr_destroy(*attr);
    *attr = NULL;
  }

  rc = policer_find(name, &conf);
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
policer_get_bandwidth_limit(const policer_attr_t *attr,
                            uint64_t *bandwidth_limit) {
  if (attr != NULL && bandwidth_limit != NULL) {
    *bandwidth_limit = attr->bandwidth_limit;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
policer_get_burst_size_limit(const policer_attr_t *attr,
                             uint64_t *burst_size_limit) {
  if (attr != NULL && burst_size_limit != NULL) {
    *burst_size_limit = attr->burst_size_limit;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
policer_get_bandwidth_percent(const policer_attr_t *attr,
                              uint8_t *bandwidth_percent) {
  if (attr != NULL && bandwidth_percent != NULL) {
    *bandwidth_percent = attr->bandwidth_percent;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
policer_get_action_names(const policer_attr_t *attr,
                         datastore_name_info_t **action_names) {
  if (attr != NULL && action_names != NULL) {
    return datastore_names_duplicate(attr->action_names,
                                     action_names, NULL);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
policer_set_bandwidth_limit(policer_attr_t *attr,
                            const uint64_t bandwidth_limit) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (bandwidth_limit -
                             MINIMUM_BANDWIDTH_LIMIT);
    long long int max_diff = (long long int) (bandwidth_limit -
                             MAXIMUM_BANDWIDTH_LIMIT);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->bandwidth_limit = bandwidth_limit;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
policer_set_burst_size_limit(policer_attr_t *attr,
                             const uint64_t burst_size_limit) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (burst_size_limit -
                             MINIMUM_BURST_SIZE_LIMIT);
    long long int max_diff = (long long int) (burst_size_limit -
                             MAXIMUM_BURST_SIZE_LIMIT);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->burst_size_limit = burst_size_limit;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
policer_set_bandwidth_percent(policer_attr_t *attr,
                              const uint64_t bandwidth_percent) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (bandwidth_percent -
                             MINIMUM_BANDWIDTH_PERCENT);
    long long int max_diff = (long long int) (bandwidth_percent -
                             MAXIMUM_BANDWIDTH_PERCENT);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->bandwidth_percent = (uint8_t) bandwidth_percent;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
policer_set_action_names(policer_attr_t *attr,
                         const datastore_name_info_t *action_names) {
  if (attr != NULL && action_names != NULL) {
    return datastore_names_duplicate(action_names,
                                     &(attr->action_names), NULL);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static void
policer_conf_freeup(void *conf) {
  policer_conf_destroy((policer_conf_t *) conf);
}

static void
policer_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&policer_table);
}

static lagopus_result_t
policer_initialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  if ((r = lagopus_hashmap_create(&policer_table, LAGOPUS_HASHMAP_TYPE_STRING,
                                  policer_conf_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }

  (void)pthread_atfork(NULL, NULL, policer_child_at_fork);

done:
  return r;

}

static void
policer_finalize(void) {
  lagopus_hashmap_destroy(&policer_table, true);
}

//
// internal datastore
//

bool
policer_exists(const char *name) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;

  rc = policer_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK && conf != NULL) {
    return true;
  }
  return false;
}

lagopus_result_t
policer_set_used(const char *name, bool is_used) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = policer_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_used = is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
policer_set_enabled(const char *name, bool is_enabled) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = policer_find(name, &conf);
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
datastore_policer_is_enabled(const char *name, bool *is_enabled) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_enabled != NULL) {
    rc = policer_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_enabled = conf->is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return rc;
}

lagopus_result_t
datastore_policer_is_used(const char *name, bool *is_used) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_used != NULL) {
    rc = policer_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_used = conf->is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_policer_get_bandwidth_limit(const char *name, bool current,
                                      uint64_t *bandwidth_limit) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && bandwidth_limit != NULL) {
    rc = policer_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = policer_get_bandwidth_limit(attr, bandwidth_limit);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_policer_get_burst_size_limit(const char *name, bool current,
                                       uint64_t *burst_size_limit) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && burst_size_limit != NULL) {
    rc = policer_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = policer_get_burst_size_limit(attr, burst_size_limit);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_policer_get_bandwidth_percent(const char *name, bool current,
                                        uint8_t *bandwidth_percent) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && bandwidth_percent != NULL) {
    rc = policer_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = policer_get_bandwidth_percent(attr, bandwidth_percent);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_policer_get_action_names(const char *name,
                                   bool current,
                                   datastore_name_info_t **action_names) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && action_names != NULL) {
    rc = policer_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = policer_get_action_names(attr, action_names);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}
