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
#include "ns_util.h"

#define MINIMUM_DST_PORT 0
#define MAXIMUM_DST_PORT UINT16_MAX
#define MINIMUM_LOCAL_PORT 0
#define MAXIMUM_LOCAL_PORT UINT16_MAX

typedef struct channel_attr {
  lagopus_ip_address_t *dst_addr;
  uint16_t dst_port;
  lagopus_ip_address_t *local_addr;
  uint16_t local_port;
  datastore_channel_protocol_t protocol;
} channel_attr_t;

typedef struct channel_conf {
  const char *name;
  channel_attr_t *current_attr;
  channel_attr_t *modified_attr;
  bool is_used;
  bool is_enabled;
  bool is_destroying;
  bool is_enabling;
  bool is_disabling;
} channel_conf_t;

typedef struct channel_protocol {
  const datastore_channel_protocol_t protocol;
  const char *protocol_str;
} channel_protocol_t;

static const channel_protocol_t protocols[] = {
  {DATASTORE_CHANNEL_PROTOCOL_UNKNOWN, "unknown"},
  {DATASTORE_CHANNEL_PROTOCOL_TCP, "tcp"},
  {DATASTORE_CHANNEL_PROTOCOL_TLS, "tls"}
};

static lagopus_hashmap_t channel_table = NULL;

static inline void
channel_attr_destroy(channel_attr_t *attr) {
  if (attr != NULL) {
    lagopus_ip_address_destroy(attr->dst_addr);
    lagopus_ip_address_destroy(attr->local_addr);
    free((void *) attr);
  }
}

static inline lagopus_result_t
channel_attr_create(channel_attr_t **attr) {
  lagopus_result_t rc;

  lagopus_ip_address_t *dst_addr = NULL;
  lagopus_ip_address_t *local_addr = NULL;

  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    channel_attr_destroy(*attr);
  }

  if ((*attr = (channel_attr_t *) malloc(sizeof(channel_attr_t)))
      == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  rc = lagopus_ip_address_create("127.0.0.1", true, &dst_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*attr)->dst_addr = dst_addr;
  (*attr)->dst_port = 6633;
  rc = lagopus_ip_address_create("0.0.0.0", true, &local_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*attr)->local_addr = local_addr;
  (*attr)->local_port = 0;
  (*attr)->protocol = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;

  return LAGOPUS_RESULT_OK;

error:
  free((void *) dst_addr);
  free((void *) local_addr);
  free((void *) *attr);
  *attr = NULL;
  return rc;
}

static inline lagopus_result_t
channel_attr_duplicate(const channel_attr_t *src_attr,
                       channel_attr_t **dst_attr, const char *namespace) {
  lagopus_result_t rc;

  (void)namespace;

  if (src_attr == NULL || dst_attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_attr != NULL) {
    channel_attr_destroy(*dst_attr);
    *dst_attr = NULL;
  }

  rc = channel_attr_create(dst_attr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  rc = lagopus_ip_address_copy(src_attr->dst_addr,
                               &((*dst_attr)->dst_addr));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*dst_attr)->dst_port = src_attr->dst_port;

  rc = lagopus_ip_address_copy(src_attr->local_addr,
                               &((*dst_attr)->local_addr));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*dst_attr)->local_port = src_attr->local_port;

  (*dst_attr)->protocol = src_attr->protocol;

  return LAGOPUS_RESULT_OK;

error:
  channel_attr_destroy(*dst_attr);
  *dst_attr = NULL;
  return rc;
}

static inline bool
channel_attr_equals(channel_attr_t *attr0, channel_attr_t *attr1) {
  if (attr0 == NULL && attr1 == NULL) {
    return true;
  } else if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((lagopus_ip_address_equals(attr0->dst_addr,
                                 attr1->dst_addr) == true) &&
      (attr0->dst_port == attr1->dst_port) &&
      (lagopus_ip_address_equals(attr0->local_addr,
                                 attr1->local_addr) == true) &&
      (attr0->local_port == attr1->local_port) &&
      (attr0->protocol == attr1->protocol)) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
channel_conf_create(channel_conf_t **conf, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  channel_attr_t *default_modified_attr = NULL;

  if (conf != NULL && IS_VALID_STRING(name) == true) {
    char *cname = strdup(name);
    if (IS_VALID_STRING(cname) == true) {
      *conf = (channel_conf_t *) malloc(sizeof(channel_conf_t));
      ret = channel_attr_create(&default_modified_attr);
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
channel_conf_destroy(channel_conf_t *conf) {
  if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
    free((void *) (conf->name));
    if (conf->current_attr != NULL) {
      channel_attr_destroy(conf->current_attr);
    }
    if (conf->modified_attr != NULL) {
      channel_attr_destroy(conf->modified_attr);
    }
  }
  free((void *) conf);
}

static inline lagopus_result_t
channel_conf_duplicate(const channel_conf_t *src_conf,
                       channel_conf_t **dst_conf, const char *namespace) {
  lagopus_result_t rc;
  channel_attr_t *dst_current_attr = NULL;
  channel_attr_t *dst_modified_attr = NULL;
  size_t len = 0;
  char *buf = NULL;

  if (src_conf == NULL || dst_conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_conf != NULL) {
    channel_conf_destroy(*dst_conf);
    *dst_conf = NULL;
  }

  if (namespace == NULL) {
    rc = channel_conf_create(dst_conf, src_conf->name);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  } else {
    if ((len = strlen(src_conf->name)) <= DATASTORE_CHANNEL_FULLNAME_MAX) {
      rc = ns_replace_namespace(src_conf->name, namespace, &buf);
      if (rc == LAGOPUS_RESULT_OK) {
        rc = channel_conf_create(dst_conf, buf);
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
    rc = channel_attr_duplicate(src_conf->current_attr,
                                &dst_current_attr, namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->current_attr = dst_current_attr;

  if (src_conf->modified_attr != NULL) {
    rc = channel_attr_duplicate(src_conf->modified_attr,
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
  channel_conf_destroy(*dst_conf);
  *dst_conf = NULL;
  return rc;
}

static inline bool
channel_conf_equals(const channel_conf_t *conf0,
                    const channel_conf_t *conf1) {
  if (conf0 == NULL && conf1 == NULL) {
    return true;
  } else if (conf0 == NULL || conf1 == NULL) {
    return false;
  }

  if ((channel_attr_equals(conf0->current_attr,
                           conf1->current_attr) == true) &&
      (channel_attr_equals(conf0->modified_attr,
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
channel_conf_add(channel_conf_t *conf) {
  if (channel_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      void *val = (void *) conf;
      return lagopus_hashmap_add(&channel_table,
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
channel_conf_delete(channel_conf_t *conf) {
  if (channel_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      return lagopus_hashmap_delete(&channel_table, (void *) conf->name,
                                    NULL, true);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

typedef struct {
  channel_conf_t **m_configs;
  size_t m_n_configs;
  size_t m_max;
  const char *m_namespace;
} channel_conf_iter_ctx_t;

static bool
channel_conf_iterate(void *key, void *val, lagopus_hashentry_t he,
                     void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;
  char *prefix = NULL;
  size_t len = 0;
  bool result = false;

  char *fullname = (char *)key;
  channel_conf_iter_ctx_t *ctx =
    (channel_conf_iter_ctx_t *)arg;

  (void)he;

  if (((channel_conf_t *) val)->is_destroying == false) {
    if (ctx->m_namespace == NULL) {
      if (ctx->m_n_configs < ctx->m_max) {
        ctx->m_configs[ctx->m_n_configs++] =
          (channel_conf_t *)val;
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
                (channel_conf_t *)val;
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
channel_conf_list(channel_conf_t ***list, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (channel_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  }

  if (list != NULL) {
    size_t n = (size_t) lagopus_hashmap_size(&channel_table);
    *list = NULL;
    if (n > 0) {
      channel_conf_t **configs =
        (channel_conf_t **) malloc(sizeof(channel_conf_t *) * n);
      if (configs != NULL) {
        channel_conf_iter_ctx_t ctx;
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

        ret = lagopus_hashmap_iterate(&channel_table, channel_conf_iterate,
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
channel_conf_one_list(channel_conf_t ***list,
                      channel_conf_t *config) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (list != NULL && config != NULL) {
    channel_conf_t **configs = (channel_conf_t **)
                               malloc(sizeof(channel_conf_t *));
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
channel_protocol_to_str(const datastore_channel_protocol_t protocol,
                        const char **protocol_str) {
  if (protocol_str == NULL ||
      (int) protocol < DATASTORE_CHANNEL_PROTOCOL_MIN ||
      (int) protocol > DATASTORE_CHANNEL_PROTOCOL_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *protocol_str = protocols[protocol].protocol_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
channel_protocol_to_enum(const char *protocol_str,
                         datastore_channel_protocol_t *protocol) {
  size_t i = 0;

  if (IS_VALID_STRING(protocol_str) != true || protocol == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_CHANNEL_PROTOCOL_MIN;
       i <= DATASTORE_CHANNEL_PROTOCOL_MAX; i++) {
    if (strcmp(protocol_str, protocols[i].protocol_str) == 0) {
      *protocol = protocols[i].protocol;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_find(const char *name, channel_conf_t **conf) {
  if (channel_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  } else if (conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (IS_VALID_STRING(name) == true) {
    return lagopus_hashmap_find(&channel_table,
                                (void *)name, (void **)conf);
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

static inline lagopus_result_t
channel_get_attr(const char *name, bool current, channel_attr_t **attr) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;

  if (name == NULL || attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    channel_attr_destroy(*attr);
    *attr = NULL;
  }

  rc = channel_find(name, &conf);
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
channel_get_dst_addr(const channel_attr_t *attr,
                     lagopus_ip_address_t **dst_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && dst_addr != NULL && *dst_addr == NULL) {
    ret = lagopus_ip_address_copy(attr->dst_addr, dst_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
channel_get_dst_addr_str(const channel_attr_t *attr, char **dst_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && dst_addr != NULL && *dst_addr == NULL) {
    ret = lagopus_ip_address_str_get(attr->dst_addr, dst_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
channel_get_dst_port(const channel_attr_t *attr, uint16_t *dst_port) {
  if (attr != NULL && dst_port != NULL) {
    *dst_port = attr->dst_port;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_get_local_addr(const channel_attr_t *attr,
                       lagopus_ip_address_t **local_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && local_addr != NULL) {
    ret = lagopus_ip_address_copy(attr->local_addr, local_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
channel_get_local_addr_str(const channel_attr_t *attr, char **local_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && local_addr != NULL) {
    ret = lagopus_ip_address_str_get(attr->local_addr, local_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
channel_get_local_port(const channel_attr_t *attr, uint16_t *local_port) {
  if (attr != NULL && local_port != NULL) {
    *local_port = attr->local_port;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_get_protocol(const channel_attr_t *attr,
                     datastore_channel_protocol_t *protocol) {
  if (attr != NULL && protocol != NULL) {
    *protocol = attr->protocol;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_set_dst_addr(channel_attr_t *attr, lagopus_ip_address_t *dst_addr) {
  if (attr != NULL && dst_addr != NULL) {
    return lagopus_ip_address_copy(dst_addr, &(attr->dst_addr));
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_set_dst_addr_str(channel_attr_t *attr,
                         const char *dst_addr, const bool prefer) {
  lagopus_result_t rc;
  size_t len = 0;

  if (attr != NULL && dst_addr != NULL) {
    len = strlen(dst_addr);
    if (len > 0 && len <= DATASTORE_INTERFACE_FULLNAME_MAX) {
      if (attr->dst_addr != NULL) {
        lagopus_ip_address_destroy(attr->dst_addr);
      }

      rc = lagopus_ip_address_create(dst_addr, prefer, &(attr->dst_addr));

      return rc;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_set_dst_port(channel_attr_t *attr, const uint64_t dst_port) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (dst_port - MINIMUM_DST_PORT);
    long long int max_diff = (long long int) (dst_port - MAXIMUM_DST_PORT);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->dst_port = (uint16_t) dst_port;
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
channel_set_local_addr(channel_attr_t *attr,
                       lagopus_ip_address_t *local_addr) {
  if (attr != NULL && local_addr != NULL) {
    return lagopus_ip_address_copy(local_addr, &(attr->local_addr));
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_set_local_addr_str(channel_attr_t *attr,
                           const char *local_addr, const bool prefer) {
  lagopus_result_t rc;
  size_t len = 0;

  if (attr != NULL && local_addr != NULL) {
    len = strlen(local_addr);
    if (len > 0 && len <= DATASTORE_INTERFACE_FULLNAME_MAX) {
      if (attr->local_addr != NULL) {
        lagopus_ip_address_destroy(attr->local_addr);
      }

      rc = lagopus_ip_address_create(local_addr, prefer,
                                     &(attr->local_addr));

      return rc;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
channel_set_local_port(channel_attr_t *attr, const uint64_t local_port) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (local_port - MINIMUM_LOCAL_PORT);
    long long int max_diff = (long long int) (local_port - MAXIMUM_LOCAL_PORT);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->local_port = (uint16_t) local_port;
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
channel_set_protocol(channel_attr_t *attr,
                     const datastore_channel_protocol_t protocol) {
  if (attr != NULL) {
    attr->protocol = protocol;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static void
channel_conf_freeup(void *conf) {
  channel_conf_destroy((channel_conf_t *) conf);
}

static void
channel_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&channel_table);
}

static lagopus_result_t
channel_initialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  if ((r = lagopus_hashmap_create(&channel_table, LAGOPUS_HASHMAP_TYPE_STRING,
                                  channel_conf_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }

  (void)pthread_atfork(NULL, NULL, channel_child_at_fork);

done:
  return r;
}

static void
channel_finalize(void) {
  lagopus_hashmap_destroy(&channel_table, true);
}

//
// internal datastore
//

bool
channel_exists(const char *name) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;

  rc = channel_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK && conf != NULL) {
    return true;
  }
  return false;
}

lagopus_result_t
channel_set_used(const char *name, bool is_used) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = channel_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_used = is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
channel_set_enabled(const char *name, bool is_enabled) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = channel_find(name, &conf);
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
datastore_channel_is_enabled(const char *name, bool *is_enabled) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_enabled != NULL) {
    rc = channel_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_enabled = conf->is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return rc;
}

lagopus_result_t
datastore_channel_is_used(const char *name, bool *is_used) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_used != NULL) {
    rc = channel_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_used = conf->is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_channel_get_dst_addr(const char *name, bool current,
                               lagopus_ip_address_t **dst_addr) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && dst_addr != NULL) {
    rc = channel_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = channel_get_dst_addr(attr, dst_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_channel_get_dst_addr_str(const char *name, bool current,
                                   char **dst_addr) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && dst_addr != NULL) {
    rc = channel_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = channel_get_dst_addr_str(attr, dst_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_channel_get_dst_port(const char *name, bool current,
                               uint16_t *dst_port) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && dst_port != NULL) {
    rc = channel_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = channel_get_dst_port(attr, dst_port);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_channel_get_local_addr(const char *name, bool current,
                                 lagopus_ip_address_t **local_addr) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && local_addr != NULL) {
    rc = channel_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = channel_get_local_addr(attr, local_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_channel_get_local_addr_str(const char *name, bool current,
                                     char **local_addr) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && local_addr != NULL) {
    rc = channel_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = channel_get_local_addr_str(attr, local_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_channel_get_local_port(const char *name, bool current,
                                 uint16_t *local_port) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && local_port != NULL) {
    rc = channel_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = channel_get_local_port(attr, local_port);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_channel_get_protocol(const char *name, bool current,
                               datastore_channel_protocol_t *protocol) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && protocol != NULL) {
    rc = channel_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = channel_get_protocol(attr, protocol);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

