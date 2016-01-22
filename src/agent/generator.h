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

/* Encode/decode code generator. */

#include <stdint.h>

#include "lagopus/queue.h"

enum decl_type {
  DECL_TYPE_UNKNOWN,
  DECL_TYPE_CHAR,
  DECL_TYPE_UCHAR,
  DECL_TYPE_WORD,
  DECL_TYPE_LONG,
  DECL_TYPE_LLONG,
};

enum exp_type {
  EXP_TYPE_DECLARATION,
  EXP_TYPE_ARRAY_DECLARATION,
  EXP_TYPE_STRUCT_DECLARATION,
  EXP_TYPE_ARRAY_STRUCT_DECLARATION,
  EXP_TYPE_ENUM_DECLARATION,
  EXP_TYPE_LIST,
  EXP_TYPE_ENUM,
};

/* Declaration.  */
struct decl {
  char *type;
  char *name;
  int array_size;
};

/* Expression list.  */
struct exp;

TAILQ_HEAD(exp_tailq, exp);

struct exp_list {
  struct exp_tailq tailq;
  uint32_t count;
  char *name;
};

/* Openflow parser generator.  */
struct exp {
  TAILQ_ENTRY(exp) entry;

  enum exp_type type;

  union {
    char *identifier;
    struct exp_list elist;
    struct decl decl;
  };
};

/* Function prototypes.  */
struct exp *
create_statement(void);

struct exp *
create_decl_expression(char *, char *);

struct exp *
create_decl_array_expression(char *, char *, int);

struct exp *
create_decl_array_id_expression(char *, char *, char *);

struct exp *
create_decl_struct_expression(char *, char *);

struct exp *
create_decl_array_struct_expression(char *, char *, int);

struct exp *
create_decl_enum_expression(char *name);

struct exp *
create_struct_expression(char *);

struct exp *
create_enum_expression(char *enum_name);

void
output_top(const char *hname);

void
output_bottom(void);

/* Global variables.  */
FILE *fp_source;
FILE *fp_header;
