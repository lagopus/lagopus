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


%{
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "confsys.h"

struct vector *stack = NULL;
struct node *current_node = NULL;

enum node_type
{
  NODE_TYPE_ONE = 1,
  NODE_TYPE_TWO = 2,
  NODE_TYPE_THREE = 3
};

TAILQ_HEAD(node_list, node);

struct node
{
  TAILQ_ENTRY(node) entry;

  enum node_type type;
  char *one;
  char *two;
  char *three;

  struct node *parent;

  struct node_list node_list;
};

static struct node *
node_alloc(const char *one, const char *two, const char *three)
{
  struct node *node;

  node = calloc(1, sizeof(struct node));
  if (node == NULL)
    return NULL;

  if (one) {
    node->one = strdup(one);
    node->type = NODE_TYPE_ONE;
  }
  if (two) {
    node->two = strdup(two);
    node->type = NODE_TYPE_TWO;
  }
  if (three) {
    node->three = strdup(three);
    node->type = NODE_TYPE_THREE;
  }

  TAILQ_INIT(&node->node_list);

  return node;
}

static void
node_free(struct node *node)
{
  struct node *child;

  if (node->one)
    free(node->one);
  if (node->two)
    free(node->two);
  if (node->three)
    free(node->three);

  while ((child = TAILQ_FIRST(&node->node_list)) != NULL) {
    TAILQ_REMOVE(&node->node_list, child, entry);
    node_free(child);
  }

  free(node);
}

static void
stack_free(struct vector *v)
{
  vindex_t i;
  struct node *node;

  for (i = 0; i < vector_max(v); i++) {
    node = vector_slot(v, i);
    node_free(node);
  }
  vector_free(v);
}

static struct node *
node_set(struct node *node, const char *one, const char *two)
{
  if (one)
    node->one = strdup(one);
  if (two)
    node->two = strdup(two);
  return node;
}

static struct node *
node_append(struct node *node, struct node *child)
{
  child->parent = node;
  TAILQ_INSERT_TAIL(&node->node_list, child, entry);
  return node;
}

int yyerror(char const *str);
%}
%union {
  char *identifier;
}
%token <identifier> IDENTIFIER
%token LC_T RC_T
%token SEMICOLON_T
%%
statement_list
: statement
{
  struct node *node;
  node = node_alloc(NULL, NULL, NULL);
  node_append(node, current_node);
  vector_set(stack, node);
}
| statement_list statement
{
  struct node *node;
  node = vector_last(stack);
  node_append(node, current_node);
}
;

statement
: IDENTIFIER LC_T statement_list RC_T
{
  current_node = vector_pop(stack);
  node_set(current_node, $1, NULL);
}
| IDENTIFIER IDENTIFIER LC_T statement_list RC_T
{
  current_node = vector_pop(stack);
  node_set(current_node, $1, $2);
}
| IDENTIFIER SEMICOLON_T
{
  current_node = node_alloc($1, NULL, NULL);
}
| IDENTIFIER IDENTIFIER SEMICOLON_T
{
  current_node = node_alloc($1, $2, NULL);
}
| IDENTIFIER IDENTIFIER IDENTIFIER SEMICOLON_T
{
  current_node = node_alloc($1, $2, $3);
}
;
%%
extern char *yytext;
extern FILE *yyin;

int
yyerror(char const *str)
{
  fprintf(stderr, "Parser error at %s: %s\n", yytext, str);
  return -1;
}

static void
config_set_write(struct node *node, struct pbuf *pbuf)
{
  if (node->parent)
    config_set_write(node->parent, pbuf);
  if (node->one) {
    ENCODE_PUT(node->one, strlen(node->one));
    ENCODE_PUTC(' ');
  }
  if (node->two) {
    ENCODE_PUT(node->two, strlen(node->two));
    ENCODE_PUTC(' ');
  }
  if (node->three) {
    ENCODE_PUT(node->three, strlen(node->three));
    ENCODE_PUTC(' ');
  }
}

static void
config_set_node(struct clink *clink, struct node *node,
                struct pbuf *pbuf, int depth)
{
  struct node *child;

  if (node->one) {
    pbuf_reset(pbuf);
    config_set_write(node, pbuf);
    (*pbuf->putp) = '\0';
    /* printf("[%s]\n", pbuf->getp); */
    clink_process(clink, CONFSYS_MSG_TYPE_SET, (char *)pbuf->getp);
  }

  TAILQ_FOREACH(child, &node->node_list, entry) {
    config_set_node(clink, child, pbuf, depth + 1);
  }
}

static void
config_set_all(struct cserver *cserver)
{
  vindex_t i;
  struct node *node;
  struct pbuf *pbuf;
  struct clink *clink;

  pbuf = pbuf_alloc(4096);
  clink = clink_alloc();
  clink->cserver = cserver;

  for (i = 0; i < vector_max(stack); i++) {
    node = vector_slot(stack, i);
    config_set_node(clink, node, pbuf, 0);
  }
  pbuf_free(pbuf);
  clink_free(clink);
}

/* Load lagopus.conf file. */
int
cserver_load_lagopus_conf(struct cserver *cserver)
{
  int ret;

  yyin = fopen(CONFSYS_CONFIG_FILENAME, "r");
  if (! yyin) {
    yyin = fopen(CONFSYS_CONFIG_FILE, "r");
    if (! yyin) {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
  }

  stack = vector_alloc();
  if (stack == NULL) {
    LAGOPUS_RESULT_NO_MEMORY;
  }

  ret = yyparse();
  if (ret != 0) {
    stack_free(stack);
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  config_set_all(cserver);

  stack_free(stack);

  return LAGOPUS_RESULT_OK;
}


/* Propagate lagopus.conf. */
int
cserver_propagate_lagopus_conf(struct cserver *cserver)
{
  cnode_child_free(cserver->config);
  cserver->config = cnode_alloc();
  return cserver_load_lagopus_conf(cserver);
}
