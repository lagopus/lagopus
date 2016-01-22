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

#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "ns_util.h"

#define MINIMUM_ID 0LL
#define MAXIMUM_ID UINT32_MAX
#define MINIMUM_PRIORITY 0LL
#define MAXIMUM_PRIORITY UINT16_MAX
#define MINIMUM_COMMITTED_BURST_SIZE 1500LL
#define MAXIMUM_COMMITTED_BURST_SIZE 100000000000LL
#define MINIMUM_COMMITTED_INFORMATION_RATE 1500LL
#define MAXIMUM_COMMITTED_INFORMATION_RATE 100000000000LL
#define MINIMUM_EXCESS_BURST_SIZE 1500LL
#define MAXIMUM_EXCESS_BURST_SIZE 100000000000LL
#define MINIMUM_PEAK_BURST_SIZE 1500LL
#define MAXIMUM_PEAK_BURST_SIZE 100000000000LL
#define MINIMUM_PEAK_INFORMATION_RATE 1500LL
#define MAXIMUM_PEAK_INFORMATION_RATE 100000000000LL

#ifndef DEFAULT_COMMITTED_BURST_SIZE
#define DEFAULT_COMMITTED_BURST_SIZE 1500LL
#endif /* DEFAULT_COMMITTED_BURST_SIZE */

#ifndef DEFAULT_COMMITTED_INFORMATION_RATE
#define DEFAULT_COMMITTED_INFORMATION_RATE 1500LL
#endif /* DEFAULT_COMMITTED_INFORMATION_RATE */

#ifndef DEFAULT_EXCESS_BURST_SIZE
#define DEFAULT_EXCESS_BURST_SIZE 1500LL
#endif /* DEFAULT_EXCESS_BURST_SIZE */

#ifndef DEFAULT_PEAK_BURST_SIZE
#define DEFAULT_PEAK_BURST_SIZE 1500LL
#endif /* DEFAULT_PEAK_BURST_SIZE */

#ifndef DEFAULT_PEAK_INFORMATION_RATE
#define DEFAULT_PEAK_INFORMATION_RATE 1500LL
#endif /* DEFAULT_PEAK_INFORMATION_RATE */

typedef struct queue_attr {
  uint32_t id;
  uint16_t priority;
  datastore_queue_type_t type;
  datastore_queue_color_t color;
  uint64_t committed_burst_size;
  uint64_t committed_information_rate;
  uint64_t excess_burst_size;
  uint64_t peak_burst_size;
  uint64_t peak_information_rate;
} queue_attr_t;

typedef struct queue_conf {
  const char *name;
  queue_attr_t *current_attr;
  queue_attr_t *modified_attr;
  bool is_used;
  bool is_enabled;
  bool is_destroying;
  bool is_enabling;
  bool is_disabling;
} queue_conf_t;

typedef struct queue_type {
  const datastore_queue_type_t type;
  const char *type_str;
} queue_type_t;

static const queue_type_t types[] = {
  {DATASTORE_QUEUE_TYPE_UNKNOWN, "unknown"},
  {DATASTORE_QUEUE_TYPE_SINGLE_RATE, "single-rate"},
  {DATASTORE_QUEUE_TYPE_TWO_RATE, "two-rate"},
};

typedef struct queue_color {
  const datastore_queue_color_t color;
  const char *color_str;
} queue_color_t;

static const queue_color_t colors[] = {
  {DATASTORE_QUEUE_COLOR_UNKNOWN, "unknown"},
  {DATASTORE_QUEUE_COLOR_AWARE, "color-aware"},
  {DATASTORE_QUEUE_COLOR_BLIND, "color-blind"},
};

static lagopus_hashmap_t queue_table = NULL;

static inline void
queue_attr_destroy(queue_attr_t *attr) {
  if (attr != NULL) {
    free((void *) attr);
  }
}

static inline lagopus_result_t
queue_attr_create(queue_attr_t **attr) {
  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    queue_attr_destroy(*attr);
    *attr = NULL;
  }

  if ((*attr = (queue_attr_t *) malloc(sizeof(queue_attr_t)))
      == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  (*attr)->id = 0;
  (*attr)->priority = 0;
  (*attr)->type = DATASTORE_QUEUE_TYPE_UNKNOWN;
  (*attr)->color = DATASTORE_QUEUE_COLOR_UNKNOWN;
  (*attr)->committed_burst_size = DEFAULT_COMMITTED_BURST_SIZE;
  (*attr)->committed_information_rate = DEFAULT_COMMITTED_INFORMATION_RATE;
  (*attr)->excess_burst_size = DEFAULT_EXCESS_BURST_SIZE;
  (*attr)->peak_burst_size = DEFAULT_PEAK_BURST_SIZE;
  (*attr)->peak_information_rate = DEFAULT_PEAK_INFORMATION_RATE;

  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
queue_attr_duplicate(const queue_attr_t *src_attr, queue_attr_t **dst_attr,
                     const char *namespace) {
  lagopus_result_t rc;

  (void)namespace;

  if (src_attr == NULL || dst_attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_attr != NULL) {
    queue_attr_destroy(*dst_attr);
    *dst_attr = NULL;
  }

  rc = queue_attr_create(dst_attr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  (*dst_attr)->id = src_attr->id;
  (*dst_attr)->priority = src_attr->priority;
  (*dst_attr)->type = src_attr->type;
  (*dst_attr)->color = src_attr->color;
  (*dst_attr)->committed_burst_size = src_attr->committed_burst_size;
  (*dst_attr)->committed_information_rate = src_attr->committed_information_rate;
  (*dst_attr)->excess_burst_size = src_attr->excess_burst_size;
  (*dst_attr)->peak_burst_size = src_attr->peak_burst_size;
  (*dst_attr)->peak_information_rate = src_attr->peak_information_rate;

  return LAGOPUS_RESULT_OK;

error:
  queue_attr_destroy(*dst_attr);
  *dst_attr = NULL;
  return rc;
}

static inline bool
queue_attr_equals(queue_attr_t *attr0, queue_attr_t *attr1) {
  if (attr0 == NULL && attr1 == NULL) {
    return true;
  } else if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((attr0->id == attr1->id) &&
      (attr0->priority == attr1->priority) &&
      (attr0->type == attr1->type) &&
      (attr0->color == attr1->color) &&
      (attr0->committed_burst_size == attr1->committed_burst_size) &&
      (attr0->committed_information_rate ==
       attr1->committed_information_rate) &&
      (attr0->excess_burst_size == attr1->excess_burst_size) &&
      (attr0->peak_burst_size == attr1->peak_burst_size) &&
      (attr0->peak_information_rate == attr1->peak_information_rate)) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
queue_conf_create(queue_conf_t **conf, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  queue_attr_t *default_modified_attr = NULL;

  if (conf != NULL && IS_VALID_STRING(name) == true) {
    char *cname = strdup(name);
    if (IS_VALID_STRING(cname) == true) {
      *conf = (queue_conf_t *) malloc(sizeof(queue_conf_t));
      ret = queue_attr_create(&default_modified_attr);
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
queue_conf_destroy(queue_conf_t *conf) {
  if (conf != NULL) {
    free((void *) (conf->name));
    if (conf->current_attr != NULL) {
      queue_attr_destroy(conf->current_attr);
    }
    if (conf->modified_attr != NULL) {
      queue_attr_destroy(conf->modified_attr);
    }
  }
  free((void *) conf);
}

static inline lagopus_result_t
queue_conf_duplicate(const queue_conf_t *src_conf,
                     queue_conf_t **dst_conf,
                     const char *namespace) {
  lagopus_result_t rc;
  queue_attr_t *dst_current_attr = NULL;
  queue_attr_t *dst_modified_attr = NULL;
  size_t len = 0;
  char *buf = NULL;

  if (src_conf == NULL || dst_conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_conf != NULL) {
    queue_conf_destroy(*dst_conf);
    *dst_conf = NULL;
  }

  if (namespace == NULL) {
    rc = queue_conf_create(dst_conf, src_conf->name);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  } else {
    if ((len = strlen(src_conf->name)) <= DATASTORE_QUEUE_FULLNAME_MAX) {
      rc = ns_replace_namespace(src_conf->name, namespace, &buf);
      if (rc == LAGOPUS_RESULT_OK) {
        rc = queue_conf_create(dst_conf, buf);
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
    rc = queue_attr_duplicate(src_conf->current_attr, &dst_current_attr,
                              namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->current_attr = dst_current_attr;

  if (src_conf->modified_attr != NULL) {
    rc = queue_attr_duplicate(src_conf->modified_attr, &dst_modified_attr,
                              namespace);
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
  queue_conf_destroy(*dst_conf);
  *dst_conf = NULL;
  return rc;
}

static inline bool
queue_conf_equals(const queue_conf_t *conf0,
                  const queue_conf_t *conf1) {
  if (conf0 == NULL && conf1 == NULL) {
    return true;
  } else if (conf0 == NULL || conf1 == NULL) {
    return false;
  }

  if ((queue_attr_equals(conf0->current_attr,
                         conf1->current_attr) == true) &&
      (queue_attr_equals(conf0->modified_attr,
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
queue_conf_add(queue_conf_t *conf) {
  if (queue_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      void *val = (void *) conf;
      return lagopus_hashmap_add(&queue_table,
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
queue_conf_delete(queue_conf_t *conf) {
  if (queue_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      return lagopus_hashmap_delete(&queue_table, (void *) conf->name,
                                    NULL, true);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

typedef struct {
  queue_conf_t **m_configs;
  size_t m_n_configs;
  size_t m_max;
  const char *m_namespace;
} queue_conf_iter_ctx_t;

static bool
queue_conf_iterate(void *key, void *val, lagopus_hashentry_t he,
                   void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;
  char *prefix = NULL;
  size_t len = 0;
  bool result = false;

  char *fullname = (char *)key;
  queue_conf_iter_ctx_t *ctx =
    (queue_conf_iter_ctx_t *)arg;

  (void)he;
  if (((queue_conf_t *) val)->is_destroying == false) {
    if (ctx->m_namespace == NULL) {
      if (ctx->m_n_configs < ctx->m_max) {
        ctx->m_configs[ctx->m_n_configs++] =
          (queue_conf_t *)val;
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
                (queue_conf_t *)val;
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
queue_conf_list(queue_conf_t ***list, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (queue_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  }

  if (list != NULL) {
    size_t n = (size_t) lagopus_hashmap_size(&queue_table);
    *list = NULL;
    if (n > 0) {
      queue_conf_t **configs =
        (queue_conf_t **) malloc(sizeof(queue_conf_t *) * n);
      if (configs != NULL) {
        queue_conf_iter_ctx_t ctx;
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

        ret = lagopus_hashmap_iterate(&queue_table, queue_conf_iterate,
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
queue_conf_one_list(queue_conf_t ***list,
                    queue_conf_t *config) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (list != NULL && config != NULL) {
    queue_conf_t **configs = (queue_conf_t **)
                             malloc(sizeof(queue_conf_t *));
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
queue_type_to_str(const datastore_queue_type_t type,
                  const char **type_str) {
  if (type_str == NULL ||
      (int) type < DATASTORE_QUEUE_TYPE_MIN ||
      (int) type > DATASTORE_QUEUE_TYPE_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *type_str = types[type].type_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
queue_type_to_enum(const char *type_str,
                   datastore_queue_type_t *type) {
  size_t i = 0;

  if (IS_VALID_STRING(type_str) != true || type == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_QUEUE_TYPE_MIN; i <= DATASTORE_QUEUE_TYPE_MAX;
       i++) {
    if (strcmp(type_str, types[i].type_str) == 0) {
      *type = types[i].type;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_color_to_str(const datastore_queue_color_t color,
                   const char **color_str) {
  if (color_str == NULL ||
      (int) color < DATASTORE_QUEUE_COLOR_MIN ||
      (int) color > DATASTORE_QUEUE_COLOR_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *color_str = colors[color].color_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
queue_color_to_enum(const char *color_str,
                    datastore_queue_color_t *color) {
  size_t i = 0;

  if (IS_VALID_STRING(color_str) != true || color == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_QUEUE_COLOR_MIN; i <= DATASTORE_QUEUE_COLOR_MAX;
       i++) {
    if (strcmp(color_str, colors[i].color_str) == 0) {
      *color = colors[i].color;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_find(const char *name, queue_conf_t **conf) {
  if (queue_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  } else if (conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (IS_VALID_STRING(name) == true && conf != NULL) {
    return lagopus_hashmap_find(&queue_table,
                                (void *)name, (void **)conf);
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

static inline lagopus_result_t
queue_get_attr(const char *name, bool current, queue_attr_t **attr) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;

  if (name == NULL || attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    queue_attr_destroy(*attr);
    *attr = NULL;
  }

  rc = queue_find(name, &conf);
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
queue_get_id(const queue_attr_t *attr, uint32_t *id) {
  if (attr != NULL && id != NULL) {
    *id = attr->id;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_priority(const queue_attr_t *attr,
                   uint16_t *priority) {
  if (attr != NULL && priority != NULL) {
    *priority = attr->priority;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_type(const queue_attr_t *attr,
               datastore_queue_type_t *type) {
  if (attr != NULL && type != NULL) {
    *type = attr->type;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_color(const queue_attr_t *attr,
                datastore_queue_color_t *color) {
  if (attr != NULL && color != NULL) {
    *color = attr->color;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_committed_burst_size(const queue_attr_t *attr,
                               uint64_t *val) {
  if (attr != NULL && val != NULL) {
    *val = attr->committed_burst_size;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_committed_information_rate(const queue_attr_t *attr,
                                     uint64_t *val) {
  if (attr != NULL && val != NULL) {
    *val = attr->committed_information_rate;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_excess_burst_size(const queue_attr_t *attr,
                            uint64_t *val) {
  if (attr != NULL && val != NULL) {
    *val = attr->excess_burst_size;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_peak_burst_size(const queue_attr_t *attr,
                          uint64_t *val) {
  if (attr != NULL && val != NULL) {
    *val = attr->peak_burst_size;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_get_peak_information_rate(const queue_attr_t *attr,
                                uint64_t *val) {
  if (attr != NULL && val != NULL) {
    *val = attr->peak_information_rate;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_set_id(queue_attr_t *attr,
             const uint64_t id) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (id - MINIMUM_ID);
    long long int max_diff = (long long int) (id - MAXIMUM_ID);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->id = (uint32_t) id;
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
queue_set_priority(queue_attr_t *attr,
                   const uint64_t priority) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (priority - MINIMUM_PRIORITY);
    long long int max_diff = (long long int) (priority - MAXIMUM_PRIORITY);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->priority = (uint16_t) priority;
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
queue_set_type(queue_attr_t *attr,
               const datastore_queue_type_t type) {
  if (attr != NULL) {
    attr->type = type;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_set_color(queue_attr_t *attr,
                const datastore_queue_color_t color) {
  if (attr != NULL) {
    attr->color = color;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
queue_set_committed_burst_size(queue_attr_t *attr,
                               const uint64_t val) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (val - MINIMUM_COMMITTED_BURST_SIZE);
    long long int max_diff =
      (long long int) (val - MAXIMUM_COMMITTED_BURST_SIZE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->committed_burst_size = val;
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
queue_set_committed_information_rate(queue_attr_t *attr,
                                     const uint64_t val) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (val - MINIMUM_COMMITTED_INFORMATION_RATE);
    long long int max_diff =
      (long long int) (val - MAXIMUM_COMMITTED_INFORMATION_RATE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->committed_information_rate = val;
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
queue_set_excess_burst_size(queue_attr_t *attr,
                            const uint64_t val) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (val - MINIMUM_EXCESS_BURST_SIZE);
    long long int max_diff =
      (long long int) (val - MAXIMUM_EXCESS_BURST_SIZE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->excess_burst_size = val;
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
queue_set_peak_burst_size(queue_attr_t *attr,
                          const uint64_t val) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (val - MINIMUM_PEAK_BURST_SIZE);
    long long int max_diff =
      (long long int) (val - MAXIMUM_PEAK_BURST_SIZE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->peak_burst_size = val;
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
queue_set_peak_information_rate(queue_attr_t *attr,
                                const uint64_t val) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (val - MINIMUM_PEAK_INFORMATION_RATE);
    long long int max_diff =
      (long long int) (val - MAXIMUM_PEAK_INFORMATION_RATE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->peak_information_rate = val;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static void
queue_conf_freeup(void *conf) {
  queue_conf_destroy((queue_conf_t *) conf);
}

static void
queue_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&queue_table);
}

static lagopus_result_t
queue_initialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  if ((r = lagopus_hashmap_create(&queue_table, LAGOPUS_HASHMAP_TYPE_STRING,
                                  queue_conf_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }

  (void)pthread_atfork(NULL, NULL, queue_child_at_fork);

done:
  return r;

}

static void
queue_finalize(void) {
  lagopus_hashmap_destroy(&queue_table, true);
}

//
// internal datastore
//

bool
queue_exists(const char *name) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;

  rc = queue_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK && conf != NULL) {
    return true;
  }
  return false;
}

lagopus_result_t
queue_set_used(const char *name, bool is_used) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = queue_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_used = is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
queue_set_enabled(const char *name, bool is_enabled) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = queue_find(name, &conf);
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
datastore_queue_is_enabled(const char *name, bool *is_enabled) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_enabled != NULL) {
    rc = queue_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_enabled = conf->is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return rc;
}

lagopus_result_t
datastore_queue_is_used(const char *name, bool *is_used) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_used != NULL) {
    rc = queue_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_used = conf->is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_id(const char *name, bool current,
                       uint32_t *id) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && id != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_id(attr, id);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_priority(const char *name, bool current,
                             uint16_t *priority) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && priority != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_priority(attr, priority);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_type(const char *name, bool current,
                         datastore_queue_type_t *type) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && type != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_type(attr, type);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_color(const char *name, bool current,
                          datastore_queue_color_t *color) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && color != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_color(attr, color);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_committed_burst_size(const char *name,
    bool current,
    uint64_t *val) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && val != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_committed_burst_size(attr, val);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_committed_information_rate(const char *name,
    bool current,
    uint64_t *val) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && val != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_committed_information_rate(attr, val);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_excess_burst_size(const char *name,
                                      bool current,
                                      uint64_t *val) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && val != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_excess_burst_size(attr, val);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_peak_burst_size(const char *name,
                                    bool current,
                                    uint64_t *val) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && val != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_peak_burst_size(attr, val);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_queue_get_peak_information_rate(const char *name,
    bool current,
    uint64_t *val) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && val != NULL) {
    rc = queue_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = queue_get_peak_information_rate(attr, val);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}
