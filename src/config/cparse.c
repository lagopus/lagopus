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


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "confsys.h"

/* Parse result structure. */
struct cparam *
cparam_alloc(void) {
  struct cparam *cparam;

  cparam = calloc(1, sizeof(struct cparam));
  if (cparam == NULL) {
    return NULL;
  }

  cparam->args = vector_alloc();
  if (cparam->args == NULL) {
    cparam_free(cparam);
    return NULL;
  }

  cparam->matched = vector_alloc();
  if (cparam->matched == NULL) {
    cparam_free(cparam);
    return NULL;
  }

  cparam->candidate = vector_alloc();
  if (cparam->candidate == NULL) {
    cparam_free(cparam);
    return NULL;
  }

  cparam->argv = vector_alloc();
  if (cparam->argv == NULL) {
    cparam_free(cparam);
    return NULL;
  }

  return cparam;
}

void
cparam_free(struct cparam *cparam) {
  if (cparam->args) {
    vindex_t i;
    for (i = 0; i < vector_max(cparam->args); i++) {
      free(vector_slot(cparam->args, i));
    }
    vector_free(cparam->args);
  }

  if (cparam->matched) {
    vector_free(cparam->matched);
  }

  if (cparam->candidate) {
    vector_free(cparam->candidate);
  }

  if (cparam->argv) {
    vector_free(cparam->argv);
  }

  free(cparam);
}

/* Build argv from matched node. */
static void
build_argv(struct cnode *cnode, struct vector *args, vindex_t vindex,
           struct vector *argv) {
  /* Reach to top node return here. */
  if (cnode_is_top(cnode)) {
    return;
  }

  /* Set node. */
  if (CHECK_FLAG(cnode->flags, CNODE_FLAG_SET_NODE)) {
    return;
  }

  /* Delete node. */
  if (CHECK_FLAG(cnode->flags, CNODE_FLAG_DELETE_NODE)) {
    return;
  }

  /* Recursive call. */
  build_argv(cnode->parent, args, vindex - 1, argv);

  /* When cnode is keyword set it otherwise set input arg.*/
  if (cnode_is_keyword(cnode)) {
    vector_set(argv, cnode->name);
  } else {
    vector_set(argv, vector_slot(args, vindex));
  }
}

/* Parse the line. */
enum cparse_result
cparse(char *line, struct cnode *top, struct cparam *param, int exec) {
  uint32_t i;
  uint32_t j;
  char *arg;
  struct cnode *cnode;
  enum match_type current;

  struct vector *args;
  struct vector *matched;
  struct vector *candidate;
  struct vector *argv;

  /* Arguments, matched and candidate. */
  args = param->args;
  matched = param->matched;
  candidate = param->candidate;
  argv = param->argv;

  /* Lexical analysis. */
  clex(line, args);

  /* Set top candidate. */
  vector_reset(candidate);
  vector_append(candidate, top->v);

  /* Empty line. */
  if (vector_max(args) == 0) {
    return CPARSE_EMPTY_LINE;
  }

  /* Parse user input arguments. */
  for (i = 0; i < vector_max(args); i++) {
    /* Set current word to arg. */
    arg = vector_slot(args, i);

    /* Empty tail space. */
    if (strcmp(arg, "") == 0) {
      if (param->index > 0) {
        param->index--;
      }
      if (args->max > 0) {
        args->max--;
      }
      param->tail = 1;
      break;
    }

    /* Remember index. */
    param->index = i;

    /* Starting from no match. */
    current = NONE_MATCH;

    /* Rest matched vector. */
    vector_reset(matched);

    /* Match with schema. */
    for (j = 0; j < vector_max(candidate); j++) {
      enum match_type match = NONE_MATCH;

      cnode = vector_slot(candidate, j);

      if (exec == CPARSE_EXEC_MODE || exec == CPARSE_CONFIG_EXEC_MODE) {
        cnode_schema_match(cnode, arg, &match);
      }

      if (exec == CPARSE_CONFIG_MODE ||
          (exec == CPARSE_CONFIG_EXEC_MODE && match == NONE_MATCH)) {
        if (strcmp(arg, cnode->name) == 0) {
          match = KEYWORD_MATCH;
        }
      }

      if (match == NONE_MATCH) {
        continue;
      }

      if (match > current) {
        vector_reset(matched);
        current = match;
        vector_set(matched, cnode);
      } else if (match == current) {
        vector_set(matched, cnode);
      }

      if (CHECK_FLAG(cnode->flags, CNODE_FLAG_SET_NODE)) {
        SET32_FLAG(param->flags, CNODE_FLAG_SET_NODE);
      }

      if (CHECK_FLAG(cnode->flags, CNODE_FLAG_DELETE_NODE)) {
        SET32_FLAG(param->flags, CNODE_FLAG_DELETE_NODE);
      }
    }

    /* There is no match. */
    if (vector_max(matched) == 0) {
      break;
    }

    /* Update next level schema. */
    vector_reset(candidate);
    for (j = 0; j < vector_max(matched); j++) {
      cnode = vector_slot(matched, j);
      vector_append(candidate, cnode->v);
    }
  }

  /* Match result check. */
  if (vector_max(matched) == 0) {
    return CPARSE_NO_MATCH;
  } else if (vector_max(matched) > 1) {
    return CPARSE_AMBIGUOUS;
  }

  /* Parse success, set matched node. */
  param->exec = vector_first(matched);

  /* Return incomplete if the node is not leaf. */
  if (!cnode_is_leaf(param->exec)) {
    return CPARSE_INCOMPLETE;
  }

  /* Here we can build argc/argv.  This is only possible at this stage
   * since during parsing, we can't determine which node is actually
   * matched.  We allow user to abbreviate input so there could be
   * multiple candidate node during parsing.  */
  if (exec) {
    build_argv(param->exec, args, vector_max(args) - 1, argv);
  }

  /* Success. */
  return CPARSE_SUCCESS;
}
