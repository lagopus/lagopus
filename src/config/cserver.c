/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "confsys.h"
#include "chgroup.h"

static volatile bool running = false;

/* event manager for cserver only */
struct event_manager *event_manager = NULL;

/* cserver thread */
static lagopus_thread_t cserver_thread = NULL;

/* cserver thread exited flag */

static bool is_gone = false;

/* cserver singleton. */
struct cserver *cserver_singleton = NULL;

/* cserver mutex. */
pthread_rwlock_t cserver_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/* static method prototypes */
static void config_free(void);

static lagopus_result_t
loop(__UNUSED const lagopus_thread_t *t, __UNUSED void *arg) {
  struct event *event;

  while (running && (event = event_fetch(event_manager)) != NULL) {
    event_call(event);
  }

  is_gone  = true;

  return LAGOPUS_RESULT_OK;
}

/* Write lock of the configu. */
static void
config_wrlock(void) {
  pthread_rwlock_wrlock(&cserver_rwlock);
}

/* Read lock of the config. */
static void
config_rdlock(void) {
  pthread_rwlock_rdlock(&cserver_rwlock);
}

/* Unlock config. */
static void
config_unlock(void) {
  pthread_rwlock_unlock(&cserver_rwlock);
}

struct cserver *
cserver_get(void) {
  return cserver_singleton;
}

static void
config_node_create(struct cnode *schema, struct cnode *config,
                   int argc, char **argv) {
  int i;
  struct cnode *parent;
  struct cnode *cnode;

  parent = config;

  for (i = 0; i < argc; i++) {
    char *name = argv[i];

    schema = cnode_match(schema, name);
    if (schema == NULL) {
      return;
    }

    cnode = cnode_lookup(parent, name);

    if (cnode == NULL) {
      cnode = cnode_alloc();
      cnode->name = strdup(name);
      cnode->flags = schema->flags;
      cnode->callback = schema->callback;
      cnode_child_add(parent, cnode);
    }
    parent = cnode;
  }
}

/* Utility function to call callback. */
static int
config_callback(struct confsys *confsys, struct cnode *cnode,
                struct vector *argv) {
  int ret;

  if (cnode->callback)
    ret = (*cnode->callback)(confsys, (int)argv->max,
                             (const char **)argv->index);
  else {
    ret = CONFIG_NO_CALLBACK;
  }

  return ret;
}

/* Delete parent node. */
static int
config_node_delete_sub(struct cnode *cnode) {
  struct cnode *parent;

  parent = cnode->parent;
  cnode_child_delete(parent, cnode);
  cnode_free(cnode);

  if (cnode_is_value(parent) && vector_max(parent->v) == 0) {
    return config_node_delete_sub(parent);
  }
  return CONFIG_SUCCESS;
}

/* Delete the specified node. */
static int
config_node_delete(struct confsys *confsys, struct cnode *cnode,
                   struct vector *argv) {
  struct cnode *parent;
  int ret;

  /* Call callback. */
  ret = config_callback(confsys, cnode, argv);
  if (ret != CONFIG_SUCCESS) {
    return ret;
  }

  /* Delete node which belongs to child. */
  parent = cnode->parent;
  cnode_child_delete(parent, cnode);
  cnode_free(cnode);

  /* Remove parent node when it is necessary. */
  if (cnode_is_value(parent) && vector_max(parent->v) == 0) {
    return config_node_delete_sub(parent);
  }

#if 0
  /* Parent node unset check. */
  if (cnode_is_value(parent) ||
      (cnode_is_multi(parent) && vector_max(parent->v) == 0)) {
    cnode = parent;
    parent = cnode->parent;
    cnode_child_delete(parent, cnode);
  }
#endif
  return ret;
}

static int
confsys_unset(struct confsys *confsys, struct cnode *schema,
              struct cnode *config, struct vector *argv) {
  vindex_t i;
  char *name;

  /* Try to find the node to be unset. */
  for (i = 0; i < vector_max(argv); i++) {
    name = vector_slot(argv, i);

    /* Lookup schema. */
    schema = cnode_match(schema, name);
    if (schema == NULL) {
      return CONFIG_SCHEMA_ERROR;
    }

    /* Lookup config. */
    config = cnode_lookup(config, name);
    if (config == NULL) {
      return CONFIG_DOES_NOT_EXIST;
    }
  }

  /* Now current node must be leaf. */
  if (!cnode_is_leaf(schema)) {
    return CONFIG_SCHEMA_ERROR;
  }

  /* Ok, node match success. */
  return config_node_delete(confsys, config, argv);
}

static struct cnode *
cnode_lookup_with_match(struct cnode *config, struct cnode *schema) {
  vindex_t i;
  struct cnode *cnode;
  enum match_type match_type;

  if (!config) {
    return NULL;
  }

  for (i = 0; i < vector_max(config->v); i++) {
    cnode = (struct cnode *)vector_slot(config->v, i);

    cnode_schema_match(schema, cnode->name, &match_type);
    if (match_type >= WORD_MATCH) {
      return cnode;
    }
  }
  return NULL;
}

/* Set configuration. */
static int
confsys_set(struct confsys *confsys, struct cnode *schema,
            struct cnode *config, struct vector *argv) {
  vindex_t i;
  char *name;
  struct cnode *schema_top;
  struct cnode *config_top;
  int ret;

  /* Remember top node. */
  schema_top = schema;
  config_top = config;

  /* Traverse all arguments. */
  for (i = 0; i < vector_max(argv); i++) {
    /* Argument. */
    name = vector_slot(argv, i);

    /* Schema matches to argument. */
    schema = cnode_match(schema, name);
    if (schema == NULL) {
      return CONFIG_SCHEMA_ERROR;
    }

    /* Existing config lookup. */
    if (cnode_is_multi(schema->parent)) {
      /* When parent node is multi, just check exact matched
       * config. */
      config = cnode_lookup(config, name);
    } else {
      config = cnode_lookup_with_match(config, schema);
    }

    /* Leaf check. */
    if (cnode_is_leaf(schema)) {
      if (config != NULL) {
        /* If this node is leaf and the schema is not keyword, replace
         * the current config with a new one. */

        if ((i + 1) == vector_max(argv)) {
          if (schema->type == CNODE_TYPE_KEYWORD) {
            return CONFIG_ALREADY_EXIST;
          } else {
            /* replace config. */
            if (schema->callback) {
              ret = config_callback(confsys, schema, argv);
              if (ret == CONFIG_SUCCESS) {
                /**/
                cnode_replace_name(config, name);
                return CONFIG_SUCCESS;
              } else {
                return CONFIG_FAILURE;
              }
            } else {
              /**/
              cnode_replace_name(config, name);
              return CONFIG_SUCCESS;
            }
          }
        }
      } else {
        /* Call callback. */
        if (schema->callback) {
          ret = config_callback(confsys, schema, argv);
          if (ret == CONFIG_SUCCESS)
            config_node_create(schema_top, config_top,
                               (int)(i + 1), (char **)argv->index);
          else {
            return CONFIG_FAILURE;
          }
        } else {
          config_node_create(schema_top, config_top,
                             (int)(i + 1), (char **)argv->index);
          return CONFIG_SUCCESS;
        }
      }
    }
  }
  return ret;
}

/* Lookup configuration. */
static struct cnode *
confsys_lookup_config(struct cnode *config, struct cnode *schema,
                      struct vector *argv) {
  vindex_t i;
  char *name;

  /* Lookup schema and matched config. */
  for (i = 0; i < vector_max(argv); i++) {
    /* Given argument. */
    name = vector_slot(argv, i);

    /* Lookup schema. */
    schema = cnode_lookup(schema, name);
    if (!schema) {
      return NULL;
    }

    config = cnode_lookup_with_match(config, schema);
    if (!config) {
      break;
    }
  }

  return config;
}

static char *
confsys_get(struct cserver *cserver, char *path) {
  char *value;
  struct cnode *lookup;
  struct cparam *param;
  enum cparse_result result;

  /* Look up current configuration.  When there is no configuration
   * but default value is in schema, return the defalut value
   * cnode.  */
  param = cparam_alloc();
  value = NULL;

  /* Parse the command. */
  result = cparse(path, cserver->schema, param, CPARSE_CONFIG_EXEC_MODE);

  /* When there is no match. */
  if (result != CPARSE_SUCCESS) {
    cparam_free(param);
    return NULL;
  }

  /* Ok, the default is here. */
  if (cnode_has_default(param->exec)) {
    value = param->exec->default_value;
  }

  /* Lookup configuration itself. */
  lookup = confsys_lookup_config(cserver->config, cserver->schema, param->argv);
  if (lookup) {
    value = lookup->name;
  }

  /* Clean up. */
  cparam_free(param);

  /* Return value. */
  return value;
}

/* Set default value to the schema tree. */
static int
confsys_set_default(struct cserver *cserver, char *path, char *val) {
  enum cparse_result result;
  struct cparam *param;
  struct cnode *cnode;

  /* parse the path. */
  param = cparam_alloc();
  if (param == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  /* Parse the command. */
  result = cparse(path, cserver->schema, param, CPARSE_CONFIG_EXEC_MODE);

  /* When there is no match. */
  if (result != CPARSE_SUCCESS) {
    cparam_free(param);
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  /* Set the default value to the node. */
  cnode = param->exec;
  SET32_FLAG(cnode->flags, CNODE_FLAG_DEFAULT);
  if (cnode->default_value) {
    free(cnode->default_value);
  }
  cnode->default_value = strdup(val);

  /* Free cparse parameter. */
  cparam_free(param);

  /* Return success. */
  return LAGOPUS_RESULT_OK;
}

/* Send reply. */
static void
clink_reply(struct clink *clink, enum config_result_t type) {
  struct pbuf *pbuf;
  size_t length;
  ssize_t nbytes;

  if (clink->sock < 0) {
    return;
  }

  pbuf = clink->pbuf;
  pbuf_reset(pbuf);

  /* Set header length. */
  length = CONFSYS_HEADER_LEN;
  length += clink->pbuf_list->contents_size;

  /* Encode message. */
  ENCODE_PUTW(type);
  ENCODE_PUTW(length);

  /* Write the message. */
  nbytes = write(clink->sock, pbuf->data, (size_t)pbuf_readable_size(pbuf));
  if (nbytes <= 0) {
    clink_stop(clink);
  }
}

/* Process command. */
void
clink_process(struct clink *clink, enum confsys_msg_type type, char *str) {
  int ret = 0;
  struct confsys confsys;
  struct cserver *cserver;
  enum cparse_result result;
  struct cparam *param;
  struct cnode *top;
  struct cnode *exec;
  struct vector *argv;

  /* Confsys server. */
  cserver = clink->cserver;

  /* Top node based on message type. */
  switch (type) {
    case CONFSYS_MSG_TYPE_GET:
      top = cserver->show;
      break;
    case CONFSYS_MSG_TYPE_SET:
    case CONFSYS_MSG_TYPE_UNSET:
      top = cserver->schema;
      break;
    case CONFSYS_MSG_TYPE_EXEC:
      top = cserver->exec;
      break;
    case CONFSYS_MSG_TYPE_LOCK:
      ret = clink_lock(clink);
      goto out;
      break;
    case CONFSYS_MSG_TYPE_UNLOCK:
      ret = clink_unlock(clink);
      goto out;
      break;
    default:
      top = cserver->show;
      break;
  }

  /* Parse parameter. */
  param = cparam_alloc();

  /* Parse the command. */
  result = cparse(str, top, param, 1);

  /* Check the result. */
  if (result == CPARSE_SUCCESS) {
    exec = param->exec;
    argv = param->argv;

    confsys.clink = clink;
    confsys.cserver = cserver;
    confsys.type = type;

    if (type == CONFSYS_MSG_TYPE_SET) {
      ret = confsys_set(&confsys, top, cserver->config, argv);
    } else if (type == CONFSYS_MSG_TYPE_UNSET) {
      ret = confsys_unset(&confsys, top, cserver->config, argv);
    } else {
      if (exec->callback != NULL) {
        ret = (*exec->callback)(&confsys, (int)argv->max,
                                (const char **)argv->index);
      } else {
        ret = CONFIG_NO_CALLBACK;
      }
    }
  }

  /* Free parse parameter. */
  cparam_free(param);

out:
  /* Send reply. */
  clink_reply(clink, ret);

  /* If there is no show output, close link. */
  if (pbuf_list_first(clink->pbuf_list) == NULL && clink->is_show) {
    clink_stop(clink);
  }
}


static void
confsys_show(struct confsys *confsys, struct cnode *node, int depth) {
  vindex_t i;
  vindex_t j;
  struct cnode *cnode;
  struct cnode *value1;
  struct cnode *value2;

  for (i = 0; i < vector_max(node->v); i++) {
    cnode = vector_slot(node->v, i);
    if (cnode != NULL) {
      if (depth == 0) {
        show(confsys, "%s", cnode->name);
      } else {
        show(confsys, "%*c%s", depth * 4, ' ', cnode->name);
      }

      if (vector_max(cnode->v) == 0) {
        show(confsys, ";\n");
      } else {
        if (cnode_is_value(cnode)) {
          for (j = 0; j < vector_max(cnode->v); j++) {
            value1 = vector_slot(cnode->v, j);
            if (j != 0) {
              show(confsys, "%*c%s", depth * 4, ' ', cnode->name);
            }
            show(confsys, " %s", value1->name);
            if (cnode_is_value(value1)) {
              value2 = vector_first(value1->v);
              if (value2) {
                show(confsys, " %s;\n", value2->name);
              } else {
                show(confsys, ";\n");
              }
            } else {
              show(confsys, ";\n");
            }
          }
        } else {
          show(confsys, " {\n");
          confsys_show(confsys, cnode, depth + 1);
          if (depth) {
            show(confsys, "%*c}\n", depth * 4, ' ');
          } else {
            show(confsys, "}\n");
          }
        }
      }
    }
  }
}

static void
confsys_save_walker(FILE *fp, struct cnode *node, int depth) {
  vindex_t i;
  vindex_t j;
  struct cnode *cnode;
  struct cnode *value1;
  struct cnode *value2;

  for (i = 0; i < vector_max(node->v); i++) {
    cnode = vector_slot(node->v, i);
    if (cnode != NULL) {
      if (depth == 0) {
        fprintf(fp, "%s", cnode->name);
      } else {
        fprintf(fp, "%*c%s", depth * 4, ' ', cnode->name);
      }

      if (vector_max(cnode->v) == 0) {
        fprintf(fp, ";\n");
      } else {
        if (cnode_is_value(cnode)) {
          for (j = 0; j < vector_max(cnode->v); j++) {
            value1 = vector_slot(cnode->v, j);
            if (j != 0) {
              fprintf(fp, "%*c%s", depth * 4, ' ', cnode->name);
            }
            fprintf(fp, " %s", value1->name);
            if (cnode_is_value(value1)) {
              value2 = vector_first(value1->v);
              if (value2) {
                fprintf(fp, " %s;\n", value2->name);
              } else {
                fprintf(fp, ";\n");
              }
            } else {
              fprintf(fp, ";\n");
            }
          }
        } else {
          fprintf(fp, " {\n");
          confsys_save_walker(fp, cnode, depth + 1);
          if (depth) {
            fprintf(fp, "%*c}\n", depth * 4, ' ');
          } else {
            fprintf(fp, "}\n");
          }
        }
      }
    }
  }
}

static void
confsys_save(struct confsys *confsys) {
  FILE *fp;
  struct cserver *cserver;

  cserver = confsys->cserver;

  fp = fopen(CONFSYS_CONFIG_FILE, "w");
  if (!fp) {
    fprintf(stderr, "%% Can't open %s\n", CONFSYS_CONFIG_FILE);
    return;
  }

  confsys_save_walker(fp, cserver->config, 0);

  fclose(fp);
}

static void
confsys_schema_path_show(struct confsys *confsys, struct cnode *node) {
  if (node->parent != NULL) {
    confsys_schema_path_show(confsys, node->parent);
  }
  if (node->name) {
    show(confsys, "%s ", node->name);
  }
}

static void
confsys_schema_path_walker(struct confsys *confsys, struct cnode *node) {
  vindex_t i;

  if (cnode_is_leaf(node)) {
    confsys_schema_path_show(confsys, node);
    show(confsys, "\n");
  }
  for (i = 0; i < vector_max(node->v); i++) {
    confsys_schema_path_walker(confsys, vector_slot(node->v, i));
  }
}

static void
confsys_schema_path(struct confsys *confsys, struct cserver *cserver) {
  confsys_schema_path_walker(confsys, cserver->schema);
}

CALLBACK(config_show_func) {
  ARG_USED();
  confsys_show(confsys, confsys->cserver->config, 0);
  return 0;
}

CALLBACK(config_show_schema_path) {
  ARG_USED();
  confsys_schema_path(confsys, confsys->cserver);
  return 0;
}

CALLBACK(config_save_func) {
  ARG_USED();
  confsys_save(confsys);
  show(confsys, "\n%% Configuration is saved!\n\n");
  return CONFIG_SUCCESS;
}

/* Allocate configuration server. */
struct cserver *
cserver_alloc(void) {
  struct cserver *cserver;

  /* Allocate memory. */
  cserver = (struct cserver *)calloc(1, sizeof(struct cserver));
  if (!cserver) {
    return NULL;
  }

  /* Reset config and show socket. */
  cserver->sock_conf = -1;
  cserver->sock_show = -1;

  /* Initialize clink and unused list. */
  TAILQ_INIT(&cserver->clink);
  TAILQ_INIT(&cserver->unused);

  /* Create config top node. */
  cserver->config = cnode_alloc();

  /* Return cserver. */
  return cserver;
}

/* Free configuration server. */
void
cserver_free(struct cserver *cserver) {
  struct clink *clink;

  /* Free clink. */
  while ((clink = TAILQ_FIRST(&cserver->clink)) != NULL) {
    TAILQ_REMOVE(&cserver->clink, clink, entry);
    clink_stop(clink);
    clink_free(clink);
  }

  /* Free unsed clink. */
  while ((clink = TAILQ_FIRST(&cserver->unused)) != NULL) {
    TAILQ_REMOVE(&cserver->unused, clink, entry);
    clink_stop(clink);
    clink_free(clink);
  }

  /* Close socket if it is opened. */
  if (cserver->sock_conf >= 0) {
    close(cserver->sock_conf);
  }
  if (cserver->sock_show >= 0) {
    close(cserver->sock_show);
  }

  /* Free cserver. */
  free(cserver);
}

/* Load cserver schema. */
static int
cserver_load_schema(struct cserver *cserver) {
  /* Load schema. */
  cserver->show = confsys_schema_load("operational.xml");
  cserver->exec = confsys_schema_load("configuration.xml");
  cserver->schema = cnode_lookup(cserver->exec, "set");
  if (cserver->schema) {
    cnode_set_flag(cserver->schema, CNODE_FLAG_SET_NODE);
  }
  return 0;
}

/* Install cserver callbacks. */
void
cserver_install_callback(struct cserver *cserver) {
  install_callback(cserver->exec, "show", config_show_func);
  install_callback(cserver->exec, "save", config_save_func);
  install_callback(cserver->show, "show schema-path", config_show_schema_path);
}

/* Open socket and start listening configuration message. */
static int
cserver_socket_create(const char *path) {
  int ret;
  int sock;
  struct sockaddr_un sun;
  socklen_t len;

  /* UNIX domain socket. */
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    return -1;
  }

  /* Make it sure that unlink. */
  unlink(path);

  /* Prepare sockaddr_un. */
  memset(&sun, 0, sizeof(struct sockaddr_un));
  sun.sun_family = AF_UNIX;
  strncpy(sun.sun_path, path, strlen(path));
  len = (socklen_t) SUN_LEN(&sun);

  /* Bind the socket. */
  ret = bind(sock, (struct sockaddr *)&sun, len);
  if (ret < 0) {
    close(sock);
    return -1;
  }

  /* Listen the socket. */
  ret = listen(sock, 5);
  if (ret < 0) {
    close(sock);
    return -1;
  }

  /* Try to change group and mode. */
  chgroup(path, CONFSYS_GROUP_NAME);

  /* Success. */
  return sock;
}

/* Accept client connection. */
static void
cserver_accept_conf(struct event *event) {
  int sock;
  int accept_sock;
  struct cserver *cserver;
  struct clink *clink;
  struct sockaddr_un sun;
  socklen_t len;
  struct event_manager *em;

  /* Fetch arguments. */
  em = event_get_manager(event);
  cserver = event_get_arg(event);
  accept_sock = event_get_sock(event);

  /* Register accept socket. */
  cserver->accept_ev = event_register_read(em, accept_sock,
                       cserver_accept_conf, cserver);

  /* Prepare sockaddr_un. */
  memset(&sun, 0, sizeof(struct sockaddr_un));
  len = sizeof(struct sockaddr_un);

  /* Accept the socket. */
  sock = accept(accept_sock, (struct sockaddr *)&sun, &len);
  if (sock < 0) {
    return;
  }

  /* Allocate a new link to client. */
  clink = clink_get(cserver);
  if (clink == NULL) {
    close(sock);
    return;
  }

  /* Start clink. */
  clink->is_show = 0;
  clink_start(clink, sock, cserver);
}

/* Accept client connection. */
static void
cserver_accept_show(struct event *event) {
  int sock;
  int accept_sock;
  struct cserver *cserver;
  struct clink *clink;
  struct sockaddr_un sun;
  socklen_t len;
  struct event_manager *em;

  /* Fetch arguments. */
  em = event_get_manager(event);
  cserver = event_get_arg(event);
  accept_sock = event_get_sock(event);

  /* Register accept socket. */
  cserver->accept_ev = event_register_read(em, accept_sock,
                       cserver_accept_show, cserver);

  /* Prepare sockaddr_un. */
  memset(&sun, 0, sizeof(struct sockaddr_un));
  len = sizeof(struct sockaddr_un);

  /* Accept the socket. */
  sock = accept(accept_sock, (struct sockaddr *)&sun, &len);
  if (sock < 0) {
    return;
  }

  /* Allocate a new link to client. */
  clink = clink_get(cserver);
  if (clink == NULL) {
    close(sock);
    return;
  }

  /* Start clink. */
  clink->is_show = 1;
  clink_start(clink, sock, cserver);
}

/* Start cserver handling. */
int
cserver_start(struct cserver *cserver, struct event_manager *em) {
  int sock;

  /* Set cserver event manager. */
  cserver->em = em;

  /* Open config socket. */
  sock = cserver_socket_create(CONFSYS_CONFIG_PATH);
  if (sock < 0) {
    return -1;
  }
  cserver->sock_conf = sock;
  cserver->accept_ev = event_register_read(em, sock, cserver_accept_conf,
                       cserver);

  /* Open show socket. */
  sock = cserver_socket_create(CONFSYS_SHOW_PATH);
  if (sock < 0) {
    return -1;
  }
  cserver->sock_show = sock;
  cserver->accept_show = event_register_read(em, sock, cserver_accept_show,
                         cserver);

  /* Success. */
  return 0;
}

/* Load lagopus.conf file without lock . */
static lagopus_result_t
config_load_lagopus_conf_nolock(void) {
  return cserver_load_lagopus_conf(cserver_singleton);
}

/* Load lagopus.conf file. */
lagopus_result_t
config_load_lagopus_conf(void) {
  lagopus_result_t ret;

  config_wrlock();
  ret = config_load_lagopus_conf_nolock();
  config_unlock();

  return ret;
}

/* Allocate cserver singleton and initialize mutex. */
lagopus_result_t
config_handle_initialize(__UNUSED void *arg, lagopus_thread_t **thdptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;


  pthread_rwlock_wrlock(&cserver_rwlock);
  if (cserver_singleton != NULL) {
    ret = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto done;
  }

  /* Allocate cserver. */
  cserver_singleton = cserver_alloc();
  if (cserver_singleton == NULL) {
    ret = LAGOPUS_RESULT_NO_MEMORY;
    goto done;
  }

  event_manager = event_manager_alloc();
  if (event_manager == NULL) {
    cserver_free(cserver_singleton);
    cserver_singleton = NULL;
    ret = LAGOPUS_RESULT_NO_MEMORY;
    goto done;
  }

  /* Allocate lagopus_thread for cserver */
  ret = lagopus_thread_create(&cserver_thread, loop, NULL, NULL,
                              "cserver", NULL);
  if (ret != LAGOPUS_RESULT_OK) {
    cserver_free(cserver_singleton);
    cserver_singleton = NULL;
    event_manager_free(event_manager);
    event_manager = NULL;
    goto done;
  }

  *thdptr = &cserver_thread;

  /* Load schema. */
  cserver_load_schema(cserver_singleton);

  /* Load lagopus.conf. */
  ret = config_load_lagopus_conf_nolock();
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_error("FAILED (%s).\n",
                      lagopus_error_get_string(ret));
    goto done;
  }

done:
  config_unlock();

  /* Return success. */
  return ret;
}

/* Free cserver. */
static void
config_free(void) {
  /* Free cserver. */
  config_wrlock();
  cserver_free(cserver_singleton);
  cserver_singleton = NULL;
  config_unlock();

  /* Destroy mutex. */
  pthread_rwlock_destroy(&cserver_rwlock);
}

/* Propagate lagopus.conf. */
int
config_propagate_lagopus_conf(void) {
  int ret;

  config_wrlock();
  ret = cserver_propagate_lagopus_conf(cserver_singleton);
  config_unlock();

  return ret;
}

/* Install confsys callbacks. */
void
config_install_callback(void) {
  config_wrlock();
  cserver_install_callback(cserver_singleton);
  config_unlock();
}

/* Start config handling start. */
lagopus_result_t
config_handle_start(void) {
  lagopus_result_t ret;

  config_wrlock();
  if (running == true) {
    ret = LAGOPUS_RESULT_OK;
    goto done;
  }
  running = true;

  ret = cserver_start(cserver_singleton, event_manager);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  ret = lagopus_thread_start(&cserver_thread, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  config_unlock();

  return ret;
}

/* Return current config or default value. */
char *
config_get(const char *path) {
  char *value;

  config_rdlock();
  value = confsys_get(cserver_singleton, (char *)path);
  config_unlock();

  return value;
}

/* Set default value to the path in the schema. */
int
config_set_default(const char *path, char *val) {
  int ret;

  config_wrlock();
  ret = confsys_set_default(cserver_singleton, (char *)path, val);
  config_unlock();
  return ret;
}

lagopus_result_t
config_handle_shutdown(shutdown_grace_level_t level) {
  bool is_valid = false;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("shutdown called\n");

  ret = lagopus_thread_is_valid(&cserver_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return ret;
  }

  running = false;

  if (level == SHUTDOWN_RIGHT_NOW) {
    event_manager_stop(event_manager);
  }

  if (is_gone == true) {
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}

const char *
confsys_sock_path(const char *basename) {
  static char buf[MAXPATHLEN];

  /* so far, socket is created at /tmp. */
  snprintf(buf, sizeof(buf), "/tmp/%s", basename);
  return buf;
}

/* Cserver thread stop. */
lagopus_result_t
config_handle_stop(void) {
  return lagopus_thread_cancel(&cserver_thread);
}

void
config_handle_finalize(void) {
  bool is_valid = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_valid(&cserver_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }

  /* unlink file. */
  unlink(CONFSYS_CONFIG_PATH);
  unlink(CONFSYS_SHOW_PATH);

  /* Free event manager */
  event_manager_free(event_manager);
  event_manager = NULL;

  /* Free config. */
  config_free();

  lagopus_thread_destroy(&cserver_thread);

  return;
}
