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
#include "lagopus_ip_addr.h"
#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "ns_util.h"

#define MINIMUM_PORT_NUMBER 0
#define MAXIMUM_PORT_NUMBER UINT32_MAX
#define MINIMUM_DST_PORT 0
#define MAXIMUM_DST_PORT UINT32_MAX
#define MINIMUM_SRC_PORT 0
#define MAXIMUM_SRC_PORT UINT32_MAX
#define MINIMUM_NI 0
#define MAXIMUM_NI 16777215
#define MINIMUM_TTL 0
#define MAXIMUM_TTL 255
#define MINIMUM_MTU 0
#define MAXIMUM_MTU UINT16_MAX

typedef struct interface_attr {
  uint32_t port_number;
  char device[DATASTORE_INTERFACE_FULLNAME_MAX + 1];
  datastore_interface_type_t type;
  lagopus_ip_address_t *dst_addr;
  uint32_t dst_port;
  lagopus_ip_address_t *mcast_group;
  lagopus_ip_address_t *src_addr;
  uint32_t src_port;
  uint32_t ni;
  uint8_t ttl;
  mac_address_t dst_hw_addr;
  mac_address_t src_hw_addr;
  uint16_t mtu;
  lagopus_ip_address_t *ip_addr;
} interface_attr_t;

typedef struct interface_conf {
  const char *name;
  interface_attr_t *current_attr;
  interface_attr_t *modified_attr;
  bool is_used;
  bool is_enabled;
  bool is_destroying;
  bool is_enabling;
  bool is_disabling;
} interface_conf_t;

typedef struct interface_type {
  const datastore_interface_type_t type;
  const char *type_str;
} interface_type_t;

static const interface_type_t types[] = {
  {DATASTORE_INTERFACE_TYPE_UNKNOWN, "unknown"},
  {DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY, "ethernet-dpdk-phy"},
  {DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV, "ethernet-dpdk-vdev"},
  {DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK, "ethernet-rawsock"},
  {DATASTORE_INTERFACE_TYPE_GRE, "gre"},
  {DATASTORE_INTERFACE_TYPE_NVGRE, "nvgre"},
  {DATASTORE_INTERFACE_TYPE_VXLAN, "vxlan"},
  {DATASTORE_INTERFACE_TYPE_VHOST_USER, "vhost-user"}
};

static lagopus_hashmap_t interface_table = NULL;

static inline void
interface_attr_destroy(interface_attr_t *attr) {
  if (attr != NULL) {
    lagopus_ip_address_destroy(attr->dst_addr);
    lagopus_ip_address_destroy(attr->mcast_group);
    lagopus_ip_address_destroy(attr->src_addr);
    lagopus_ip_address_destroy(attr->ip_addr);
    free((void *) attr);
  }
}

static inline lagopus_result_t
interface_attr_create(interface_attr_t **attr) {
  lagopus_result_t rc;

  lagopus_ip_address_t *dst_addr = NULL;
  lagopus_ip_address_t *mcast_group = NULL;
  lagopus_ip_address_t *src_addr = NULL;
  lagopus_ip_address_t *ip_addr = NULL;
  mac_address_t default_hw_addr = {0};

  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    interface_attr_destroy(*attr);
    *attr = NULL;
  }

  if ((*attr = (interface_attr_t *) malloc(sizeof(interface_attr_t)))
      == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  (*attr)->port_number = 0;
  (*attr)->device[0] = '\0';
  (*attr)->type = DATASTORE_INTERFACE_TYPE_UNKNOWN;
  rc = lagopus_ip_address_create("127.0.0.1", true, &dst_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*attr)->dst_addr = dst_addr;
  (*attr)->dst_port = 0;
  rc = lagopus_ip_address_create("224.0.0.1", true, &mcast_group);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*attr)->mcast_group = mcast_group;
  rc = lagopus_ip_address_create("127.0.0.1", true, &src_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*attr)->src_addr = src_addr;
  (*attr)->src_port = 0;
  (*attr)->ni = 0;
  (*attr)->ttl = 0;
  rc = copy_mac_address(default_hw_addr, (*attr)->dst_hw_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    return rc;
  }
  rc = copy_mac_address(default_hw_addr, (*attr)->src_hw_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    return rc;
  }
#if MAX_PACKET_SZ > 2048
  (*attr)->mtu = 9000; /* preliminary */
#else
  (*attr)->mtu = 1500;
#endif /* MAX_PACKET_SZ */
  rc = lagopus_ip_address_create("127.0.0.1", true, &ip_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*attr)->ip_addr = ip_addr;

  return LAGOPUS_RESULT_OK;

error:
  free((void *) dst_addr);
  free((void *) mcast_group);
  free((void *) src_addr);
  free((void *) ip_addr);
  free((void *) *attr);
  *attr = NULL;
  return rc;
}

static inline lagopus_result_t
interface_attr_duplicate(const interface_attr_t *src_attr,
                         interface_attr_t **dst_attr,
                         const char *namespace) {
  lagopus_result_t rc;
  size_t len = 0;

  (void)namespace;

  if (src_attr == NULL || dst_attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_attr != NULL) {
    interface_attr_destroy(*dst_attr);
    *dst_attr = NULL;
  }

  rc = interface_attr_create(dst_attr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  (*dst_attr)->port_number = src_attr->port_number;

  if (src_attr->device != NULL) {
    if ((len = strlen(src_attr->device)) <= DATASTORE_INTERFACE_FULLNAME_MAX) {
      strncpy((*dst_attr)->device, src_attr->device, len);
      (*dst_attr)->device[len] = '\0';
    } else {
      rc = LAGOPUS_RESULT_TOO_LONG;
      goto error;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
    goto error;
  }

  (*dst_attr)->type = src_attr->type;
  rc = lagopus_ip_address_copy(src_attr->dst_addr, &((*dst_attr)->dst_addr));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*dst_attr)->dst_port = src_attr->dst_port;
  rc = lagopus_ip_address_copy(src_attr->mcast_group,
                               &((*dst_attr)->mcast_group));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  rc = lagopus_ip_address_copy(src_attr->src_addr, &((*dst_attr)->src_addr));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*dst_attr)->src_port = src_attr->src_port;
  (*dst_attr)->ni = src_attr->ni;
  (*dst_attr)->ttl = src_attr->ttl;
  rc = copy_mac_address(src_attr->dst_hw_addr, (*dst_attr)->dst_hw_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  rc = copy_mac_address(src_attr->src_hw_addr, (*dst_attr)->src_hw_addr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*dst_attr)->mtu = src_attr->mtu;
  rc = lagopus_ip_address_copy(src_attr->ip_addr, &((*dst_attr)->ip_addr));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  return LAGOPUS_RESULT_OK;

error:
  interface_attr_destroy(*dst_attr);
  *dst_attr = NULL;
  return rc;
}

static inline bool
interface_attr_equals(const interface_attr_t *attr0,
                      const interface_attr_t *attr1) {
  if (attr0 == NULL && attr1 == NULL) {
    return true;
  } else if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((attr0->port_number == attr1->port_number) &&
      (strcmp(attr0->device, attr1->device) == 0) &&
      (attr0->type == attr1->type) &&
      (lagopus_ip_address_equals(attr0->dst_addr,
                                 attr1->dst_addr) == true) &&
      (attr0->dst_port == attr1->dst_port) &&
      (lagopus_ip_address_equals(attr0->mcast_group,
                                 attr1->mcast_group) == true) &&
      (lagopus_ip_address_equals(attr0->src_addr,
                                 attr1->src_addr) == true) &&
      (attr0->src_port == attr1->src_port) &&
      (attr0->ni == attr1->ni) &&
      (attr0->ttl == attr1->ttl) &&
      (equals_mac_address(attr0->dst_hw_addr, attr1->dst_hw_addr) == true) &&
      (equals_mac_address(attr0->src_hw_addr, attr1->src_hw_addr) == true) &&
      (attr0->mtu == attr1->mtu) &&
      (lagopus_ip_address_equals(attr0->ip_addr,
                                 attr1->ip_addr) == true)) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
interface_conf_create(interface_conf_t **conf, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_attr_t *default_modified_attr = NULL;

  if (conf != NULL && IS_VALID_STRING(name) == true) {
    char *cname = strdup(name);
    if (IS_VALID_STRING(cname) == true) {
      *conf = (interface_conf_t *) malloc(sizeof(interface_conf_t));
      ret = interface_attr_create(&default_modified_attr);
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
interface_conf_destroy(interface_conf_t *conf) {
  if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
    free((void *) (conf->name));
    if (conf->current_attr != NULL) {
      interface_attr_destroy(conf->current_attr);
    }
    if (conf->modified_attr != NULL) {
      interface_attr_destroy(conf->modified_attr);
    }
  }
  free((void *) conf);
}

static inline lagopus_result_t
interface_conf_duplicate(const interface_conf_t *src_conf,
                         interface_conf_t **dst_conf,
                         const char *namespace) {
  lagopus_result_t rc;
  interface_attr_t *dst_current_attr = NULL;
  interface_attr_t *dst_modified_attr = NULL;
  size_t len = 0;
  char *buf = NULL;

  if (src_conf == NULL || dst_conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_conf != NULL) {
    interface_conf_destroy(*dst_conf);
    *dst_conf = NULL;
  }

  if (namespace == NULL) {
    rc = interface_conf_create(dst_conf, src_conf->name);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  } else {
    if ((len = strlen(src_conf->name)) <= DATASTORE_INTERFACE_FULLNAME_MAX) {
      rc = ns_replace_namespace(src_conf->name, namespace, &buf);
      if (rc == LAGOPUS_RESULT_OK) {
        rc = interface_conf_create(dst_conf, buf);
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
    rc = interface_attr_duplicate(src_conf->current_attr, &dst_current_attr,
                                  namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->current_attr = dst_current_attr;

  if (src_conf->modified_attr != NULL) {
    rc = interface_attr_duplicate(src_conf->modified_attr, &dst_modified_attr,
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
  interface_conf_destroy(*dst_conf);
  *dst_conf = NULL;
  return rc;
}

static inline bool
interface_conf_equals(const interface_conf_t *conf0,
                      const interface_conf_t *conf1) {
  if (conf0 == NULL && conf1 == NULL) {
    return true;
  } else if (conf0 == NULL || conf1 == NULL) {
    return false;
  }

  if ((interface_attr_equals(conf0->current_attr,
                             conf1->current_attr) == true) &&
      (interface_attr_equals(conf0->modified_attr,
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
interface_conf_add(interface_conf_t *conf) {
  if (interface_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      void *val = (void *) conf;
      return lagopus_hashmap_add(&interface_table,
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
interface_conf_delete(interface_conf_t *conf) {
  if (interface_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      return lagopus_hashmap_delete(&interface_table, (void *) conf->name,
                                    NULL, true);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

typedef struct {
  interface_conf_t **m_configs;
  size_t m_n_configs;
  size_t m_max;
  const char *m_namespace;
} interface_conf_iter_ctx_t;

static bool
interface_conf_iterate(void *key, void *val, lagopus_hashentry_t he,
                       void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;
  char *prefix = NULL;
  size_t len = 0;
  bool result = false;

  char *fullname = (char *)key;
  interface_conf_iter_ctx_t *ctx =
    (interface_conf_iter_ctx_t *)arg;

  (void)he;

  if (((interface_conf_t *) val)->is_destroying == false) {
    if (ctx->m_namespace == NULL) {
      if (ctx->m_n_configs < ctx->m_max) {
        ctx->m_configs[ctx->m_n_configs++] =
          (interface_conf_t *)val;
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
                (interface_conf_t *)val;
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
interface_conf_list(interface_conf_t ***list, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (interface_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  }

  if (list != NULL) {
    size_t n = (size_t) lagopus_hashmap_size(&interface_table);
    *list = NULL;
    if (n > 0) {
      interface_conf_t **configs =
        (interface_conf_t **) malloc(sizeof(interface_conf_t *) * n);
      if (configs != NULL) {
        interface_conf_iter_ctx_t ctx;
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

        ret = lagopus_hashmap_iterate(&interface_table, interface_conf_iterate,
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
interface_conf_one_list(interface_conf_t ***list,
                        interface_conf_t *config) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (list != NULL && config != NULL) {
    interface_conf_t **configs = (interface_conf_t **)
                                 malloc(sizeof(interface_conf_t *));
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
interface_type_to_str(const datastore_interface_type_t type,
                      const char **type_str) {
  if (type_str == NULL ||
      (int) type < DATASTORE_INTERFACE_TYPE_MIN ||
      (int) type > DATASTORE_INTERFACE_TYPE_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *type_str = types[type].type_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
interface_type_to_enum(const char *type_str,
                       datastore_interface_type_t *type) {
  size_t i = 0;

  if (IS_VALID_STRING(type_str) != true || type == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_INTERFACE_TYPE_MIN; i <= DATASTORE_INTERFACE_TYPE_MAX;
       i++) {
    if (strcmp(type_str, types[i].type_str) == 0) {
      *type = types[i].type;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_find(const char *name, interface_conf_t **conf) {
  if (interface_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  } else if (conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (IS_VALID_STRING(name) == true && conf != NULL) {
    return lagopus_hashmap_find(&interface_table,
                                (void *)name, (void **)conf);
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

static inline lagopus_result_t
interface_get_attr(const char *name, bool current, interface_attr_t **attr) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;

  if (name == NULL || attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    interface_attr_destroy(*attr);
    *attr = NULL;
  }

  rc = interface_find(name, &conf);
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
interface_get_port_number(const interface_attr_t *attr,
                          uint32_t *port_number) {
  if (attr != NULL && port_number != NULL) {
    *port_number = attr->port_number;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_device(const interface_attr_t *attr,
                     char **device) {
  if (attr != NULL && device != NULL) {
    *device = strdup(attr->device);
    if (*device == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_type(const interface_attr_t *attr,
                   datastore_interface_type_t *type) {
  if (attr != NULL && type != NULL) {
    *type = attr->type;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_dst_addr(const interface_attr_t *attr,
                       lagopus_ip_address_t **dst_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && dst_addr != NULL) {
    ret = lagopus_ip_address_copy(attr->dst_addr, dst_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_get_dst_addr_str(const interface_attr_t *attr, char **dst_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && dst_addr != NULL) {
    ret = lagopus_ip_address_str_get(attr->dst_addr, dst_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_get_dst_port(const interface_attr_t *attr, uint32_t *dst_port) {
  if (attr != NULL && dst_port != NULL) {
    *dst_port = attr->dst_port;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_mcast_group(const interface_attr_t *attr,
                          lagopus_ip_address_t **mcast_group) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && mcast_group != NULL) {
    ret = lagopus_ip_address_copy(attr->mcast_group, mcast_group);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_get_mcast_group_str(const interface_attr_t *attr,
                              char **mcast_group) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && mcast_group != NULL) {
    ret = lagopus_ip_address_str_get(attr->mcast_group, mcast_group);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_get_src_addr(const interface_attr_t *attr,
                       lagopus_ip_address_t **src_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && src_addr != NULL) {
    ret = lagopus_ip_address_copy(attr->src_addr, src_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_get_src_addr_str(const interface_attr_t *attr, char **src_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && src_addr != NULL) {
    ret = lagopus_ip_address_str_get(attr->src_addr, src_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_get_src_port(const interface_attr_t *attr, uint32_t *src_port) {
  if (attr != NULL && src_port != NULL) {
    *src_port = attr->src_port;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_ni(const interface_attr_t *attr, uint32_t *ni) {
  if (attr != NULL && ni != NULL) {
    *ni = attr->ni;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_ttl(const interface_attr_t *attr, uint8_t *ttl) {
  if (attr != NULL && ttl != NULL) {
    *ttl = attr->ttl;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_dst_hw_addr(const interface_attr_t *attr,
                          mac_address_t *dst_hw_addr) {
  if (attr != NULL && dst_hw_addr != NULL) {
    return copy_mac_address(attr->dst_hw_addr, *dst_hw_addr);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_src_hw_addr(const interface_attr_t *attr,
                          mac_address_t *src_hw_addr) {
  if (attr != NULL && src_hw_addr != NULL) {
    return copy_mac_address(attr->src_hw_addr, *src_hw_addr);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_mtu(const interface_attr_t *attr, uint16_t *mtu) {
  if (attr != NULL && mtu != NULL) {
    *mtu = attr->mtu;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_get_ip_addr(const interface_attr_t *attr,
                      lagopus_ip_address_t **ip_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && ip_addr != NULL) {
    ret = lagopus_ip_address_copy(attr->ip_addr, ip_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_get_ip_addr_str(const interface_attr_t *attr, char **ip_addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (attr != NULL && ip_addr != NULL) {
    ret = lagopus_ip_address_str_get(attr->ip_addr, ip_addr);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
interface_set_port_number(interface_attr_t *attr, const uint64_t port_number) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (port_number - MINIMUM_PORT_NUMBER);
    long long int max_diff = (long long int) (port_number - MAXIMUM_PORT_NUMBER);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->port_number = (uint32_t) port_number;
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
interface_set_device(interface_attr_t *attr, const char *device) {
  if (attr != NULL && device != NULL) {
    size_t len = strlen(device);
    if (len <= DATASTORE_INTERFACE_FULLNAME_MAX) {
      strncpy(attr->device, device, len);
      attr->device[len] = '\0';
      return LAGOPUS_RESULT_OK;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_type(interface_attr_t *attr,
                   const datastore_interface_type_t type) {
  if (attr != NULL) {
    attr->type = type;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_dst_addr(interface_attr_t *attr,
                       lagopus_ip_address_t *dst_addr) {
  if (attr != NULL && dst_addr != NULL) {
    return lagopus_ip_address_copy(dst_addr, &(attr->dst_addr));
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_dst_addr_str(interface_attr_t *attr,
                           const char *dst_addr, const bool prefer) {
  lagopus_result_t rc;

  if (attr != NULL && dst_addr != NULL) {
    if (attr->dst_addr != NULL) {
      lagopus_ip_address_destroy(attr->dst_addr);
    }

    rc = lagopus_ip_address_create(dst_addr, prefer, &(attr->dst_addr));

    return rc;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_dst_port(interface_attr_t *attr, const uint64_t dst_port) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (dst_port - MINIMUM_DST_PORT);
    long long int max_diff = (long long int) (dst_port - MAXIMUM_DST_PORT);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->dst_port = (uint32_t) dst_port;
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
interface_set_mcast_group(interface_attr_t *attr,
                          lagopus_ip_address_t *mcast_group) {
  if (attr != NULL && mcast_group != NULL) {
    return lagopus_ip_address_copy(mcast_group, &(attr->mcast_group));
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_mcast_group_str(interface_attr_t *attr,
                              const char *mcast_group, const bool prefer) {
  lagopus_result_t rc;

  if (attr != NULL && mcast_group != NULL) {
    if (attr->mcast_group != NULL) {
      lagopus_ip_address_destroy(attr->mcast_group);
    }

    rc = lagopus_ip_address_create(mcast_group, prefer,
                                   &(attr->mcast_group));

    return rc;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_src_addr(interface_attr_t *attr,
                       lagopus_ip_address_t *src_addr) {
  if (attr != NULL && src_addr != NULL) {
    return lagopus_ip_address_copy(src_addr, &(attr->src_addr));
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_src_addr_str(interface_attr_t *attr,
                           const char *src_addr, const bool prefer) {
  lagopus_result_t rc;

  if (attr != NULL && src_addr != NULL) {
    if (attr->src_addr != NULL) {
      lagopus_ip_address_destroy(attr->src_addr);
    }

    rc = lagopus_ip_address_create(src_addr, prefer, &(attr->src_addr));

    return rc;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_src_port(interface_attr_t *attr, uint64_t src_port) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (src_port - MINIMUM_SRC_PORT);
    long long int max_diff = (long long int) (src_port - MAXIMUM_SRC_PORT);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->src_port = (uint32_t) src_port;
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
interface_set_ni(interface_attr_t *attr, uint64_t ni) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (ni - MINIMUM_NI);
    long long int max_diff = (long long int) (ni - MAXIMUM_NI);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->ni = (uint32_t) ni;
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
interface_set_ttl(interface_attr_t *attr, uint64_t ttl) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (ttl - MINIMUM_TTL);
    long long int max_diff = (long long int) (ttl - MAXIMUM_TTL);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->ttl = (uint8_t) ttl;
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
interface_set_dst_hw_addr(interface_attr_t *attr,
                          const mac_address_t dst_hw_addr) {
  if (attr != NULL && dst_hw_addr != NULL) {
    return copy_mac_address(dst_hw_addr, attr->dst_hw_addr);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_src_hw_addr(interface_attr_t *attr,
                          const mac_address_t src_hw_addr) {
  if (attr != NULL && src_hw_addr != NULL) {
    return copy_mac_address(src_hw_addr, attr->src_hw_addr);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_mtu(interface_attr_t *attr, uint64_t mtu) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (mtu - MINIMUM_MTU);
    long long int max_diff = (long long int) (mtu - MAXIMUM_MTU);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->mtu = (uint16_t) mtu;
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
interface_set_ip_addr(interface_attr_t *attr,
                      lagopus_ip_address_t *ip_addr) {
  if (attr != NULL && ip_addr != NULL) {
    return lagopus_ip_address_copy(ip_addr, &(attr->ip_addr));
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
interface_set_ip_addr_str(interface_attr_t *attr,
                          const char *ip_addr, const bool prefer) {
  lagopus_result_t rc;

  if (attr != NULL && ip_addr != NULL) {
    if (attr->ip_addr != NULL) {
      lagopus_ip_address_destroy(attr->ip_addr);
    }

    rc = lagopus_ip_address_create(ip_addr, prefer, &(attr->ip_addr));

    return rc;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static void
interface_conf_freeup(void *conf) {
  interface_conf_destroy((interface_conf_t *) conf);
}

static void
interface_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&interface_table);
}

static lagopus_result_t
interface_initialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  if ((r = lagopus_hashmap_create(&interface_table, LAGOPUS_HASHMAP_TYPE_STRING,
                                  interface_conf_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }

  (void)pthread_atfork(NULL, NULL, interface_child_at_fork);

done:
  return r;
}

static void
interface_finalize(void) {
  lagopus_hashmap_destroy(&interface_table, true);
}

//
// internal datastore
//
bool
interface_exists(const char *name) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;

  rc = interface_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK && conf != NULL) {
    return true;
  }
  return false;
}

lagopus_result_t
interface_set_used(const char *name, bool is_used) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = interface_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_used = is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
interface_set_enabled(const char *name, bool is_enabled) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = interface_find(name, &conf);
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
datastore_interface_is_enabled(const char *name, bool *is_enabled) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_enabled != NULL) {
    rc = interface_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_enabled = conf->is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return rc;
}

lagopus_result_t
datastore_interface_is_used(const char *name, bool *is_used) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_used != NULL) {
    rc = interface_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_used = conf->is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_port_number(const char *name, bool current,
                                    uint32_t *port_number) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && port_number != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_port_number(attr, port_number);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_device(const char *name, bool current,
                               char **device) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && device != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_device(attr, device);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_type(const char *name, bool current,
                             datastore_interface_type_t *type) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && type != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_type(attr, type);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_dst_addr(const char *name, bool current,
                                 lagopus_ip_address_t **dst_addr) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true &&
      dst_addr != NULL && *dst_addr == NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_dst_addr(attr, dst_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_dst_addr_str(const char *name, bool current,
                                     char **dst_addr) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true &&
      dst_addr != NULL && *dst_addr == NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_dst_addr_str(attr, dst_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_dst_port(const char *name, bool current,
                                 uint32_t *dst_port) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && dst_port != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_dst_port(attr, dst_port);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_mcast_group(const char *name, bool current,
                                    lagopus_ip_address_t **mcast_group) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && mcast_group != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_mcast_group(attr, mcast_group);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_mcast_group_str(const char *name, bool current,
                                        char **mcast_group) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && mcast_group != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_mcast_group_str(attr, mcast_group);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_src_addr(const char *name, bool current,
                                 lagopus_ip_address_t **src_addr) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && src_addr != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_src_addr(attr, src_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_src_addr_str(const char *name, bool current,
                                     char **src_addr) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && src_addr != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_src_addr_str(attr, src_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_src_port(const char *name, bool current,
                                 uint32_t *src_port) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && src_port != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_src_port(attr, src_port);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_ni(const char *name, bool current, uint32_t *ni) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && ni != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_ni(attr, ni);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_ttl(const char *name, bool current, uint8_t *ttl) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && ttl != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_ttl(attr, ttl);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_dst_hw_addr(const char *name, bool current,
                                    uint8_t *dst_hw_addr, size_t len) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && dst_hw_addr != NULL && len == 6) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_dst_hw_addr(attr, (mac_address_t *) dst_hw_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_src_hw_addr(const char *name, bool current,
                                    uint8_t *src_hw_addr, size_t len) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && src_hw_addr != NULL && len == 6) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_src_hw_addr(attr, (mac_address_t *) src_hw_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_mtu(const char *name, bool current, uint16_t *mtu) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && mtu != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_mtu(attr, mtu);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_ip_addr(const char *name, bool current,
                                lagopus_ip_address_t **ip_addr) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && ip_addr != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_ip_addr(attr, ip_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_interface_get_ip_addr_str(const char *name, bool current,
                                    char **ip_addr) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && ip_addr != NULL) {
    rc = interface_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = interface_get_ip_addr_str(attr, ip_addr);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}
