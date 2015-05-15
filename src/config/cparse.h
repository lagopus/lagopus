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


#ifndef SRC_CONFIG_CPARSE_H_
#define SRC_CONFIG_CPARSE_H_

/* Library include. */
#include "lagopus/vector.h"

/* Parse result. */
enum cparse_result {
  CPARSE_NO_MATCH,
  CPARSE_EMPTY_LINE,
  CPARSE_INCOMPLETE,
  CPARSE_AMBIGUOUS,
  CPARSE_SUCCESS
};

/* cparse parameter. */
struct cparam {
  /* Parsed input. */
  struct vector *args;

  /* Current parsing argument index. */
  uint32_t index;

  /* Finally parsed arguments. */
  struct vector *argv;

  /* Matched. */
  struct vector *matched;

  /* Candidate schema name. */
  struct vector *candidate;

  /* Set node. */
  uint32_t flags;

  /* Tail space. */
  int tail;

  /* Execucte node. */
  struct cnode *exec;
};

struct cparam *
cparam_alloc(void);

void
cparam_free(struct cparam *param);

enum cparse_exec_match {
  CPARSE_CONFIG_MODE = 0,
  CPARSE_EXEC_MODE = 1,
  CPARSE_CONFIG_EXEC_MODE = 2
};

enum cparse_result
cparse(char *line, struct cnode *top, struct cparam *, int exec);

#endif /* SRC_CONFIG_CPARSE_H_ */
