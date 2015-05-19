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


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "confsys.h"

/* SAX user data. */
struct cnode_parse {
  struct cnode *top;
  struct vector *v;
};

/* SAX parser function for element start. */
static void
cnode_start(struct cxml_element *ele, void *user_data) {
  struct cnode_parse *parse;
  struct cnode *parent;
  struct cnode *cnode;
  const char *name;

  /* cnode parse. */
  parse = (struct cnode_parse *)user_data;

  if (strcmp(ele->name, "cnode:schema") == 0) {
    /* Top node. */
    cnode = cnode_alloc();
    parse->top = cnode;
    vector_set(parse->v, cnode);
  } else {
    /* Command node definition. */
    parent = vector_last(parse->v);
    if (parent == NULL) {
      return;
    }

    /* Command node name. */
    name = cxml_attribute_value(ele, "name");
    if (name != NULL) {
      const char *range;
      const char *help;
      const char *guide;
      const char *leaf;
      const char *multi;
      const char *value;
      const char *default_value;

      /* Allocate cnode. */
      cnode = cnode_alloc();
      cnode->name = strdup(name);

      range = cxml_attribute_value(ele, "range");
      if (range != NULL) {
        cnode->range = strdup(range);
      }

      /* Set cnode type. */
      cnode_type_set(cnode);

      help = cxml_attribute_value(ele, "help");
      if (help != NULL) {
        cnode->help = strdup(help);
      }
      guide = cxml_attribute_value(ele, "guide");
      if (guide != NULL) {
        cnode->guide = strdup(guide);
      }
      leaf = cxml_attribute_value(ele, "leaf");
      if (leaf != NULL) {
        SET32_FLAG(cnode->flags, CNODE_FLAG_LEAF);
      }
      multi = cxml_attribute_value(ele, "multi");
      if (multi != NULL) {
        SET32_FLAG(cnode->flags, CNODE_FLAG_MULTI);
      }
      value = cxml_attribute_value(ele, "value");
      if (value != NULL) {
        SET32_FLAG(cnode->flags, CNODE_FLAG_VALUE);
      }
      default_value = cxml_attribute_value(ele, "default");
      if (default_value != NULL) {
        cnode->default_value = strdup(default_value);
        SET32_FLAG(cnode->flags, CNODE_FLAG_DEFAULT);
      }

      cnode_child_add(parent, cnode);
      vector_set(parse->v, cnode);
    }
  }
}

static void
cnode_end(struct cxml_element *ele, void *user_data) {
  struct cnode_parse *parse;
  (void)ele;

  /* cnode parse. */
  parse = (struct cnode_parse *)user_data;

  /* Pop the last element from vector. */
  vector_unset_last(parse->v);
}

struct cnode *
confsys_schema_load(const char *filename) {
  struct cxml_document *doc;
  struct cnode_parse parse;

  /* Read XML defined schema. */
  doc = cxml_document_read(filename);
  if (doc == NULL) {
    fprintf(stderr, "%s read error.\n", filename);
    exit(1);
  }

  parse.v = vector_alloc();
  cxml_sax(doc, cnode_start, NULL, cnode_end, &parse);
  cnode_sort(parse.top);

  return parse.top;
}

lagopus_result_t
install_callback(struct cnode *top, const char *str, callback_t callback) {
  enum cparse_result result;
  struct cnode *match;
  struct cparam *cparam;

  cparam = cparam_alloc();
  if (cparam == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  result = cparse((char *)str, top, cparam, 0);

  if (result == CPARSE_SUCCESS) {
    match = cparam->exec;
    match->callback = callback;
    cparam_free(cparam);
    return LAGOPUS_RESULT_OK;
  } else {
    cparam_free(cparam);
    return LAGOPUS_RESULT_NOT_FOUND;
  }
}

lagopus_result_t
install_config_callback(const char *str, callback_t callback) {
  struct cserver *cserver;

  cserver = cserver_get();
  if (cserver != NULL) {
    return install_callback(cserver->config, str, callback);
  }
  return LAGOPUS_RESULT_NOT_STARTED;
}

lagopus_result_t
install_exec_callback(const char *str, callback_t callback) {
  struct cserver *cserver;

  cserver = cserver_get();
  if (cserver != NULL) {
    return install_callback(cserver->exec, str, callback);
  }
  return LAGOPUS_RESULT_NOT_STARTED;
}

lagopus_result_t
install_show_callback(const char *str, callback_t callback) {
  struct cserver *cserver;

  cserver = cserver_get();
  if (cserver != NULL) {
    return install_callback(cserver->show, str, callback);
  }
  return LAGOPUS_RESULT_NOT_STARTED;
}
