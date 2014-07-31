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
#include <string.h>
#include "shell.h"
#include "signals.h"
#include "readline.h"
#include "command.h"
#include "process.h"

#include "confsys.h"
#include "cclient.h"

/* Singleton. */
struct shell *shell;

/* Wrapper for pager. */
int
shell_config_func(void *arg, int argc, char **argv) {
  int ret;
  struct cclient *cclient;

  cclient = (struct cclient *)arg;

  ret = cclient_request_with_args(cclient, argc, argv);

  return ret;
}

/* Shell execution with more. */
static void
shell_config_more(struct cparam *param, int argc, char **argv) {
  enum confsys_msg_type type;

  if (CHECK_FLAG(param->flags, CNODE_FLAG_SET_NODE)) {
    type = CONFSYS_MSG_TYPE_SET;
  } else if (CHECK_FLAG(param->flags, CNODE_FLAG_DELETE_NODE)) {
    type = CONFSYS_MSG_TYPE_UNSET;
  } else {
    type = CONFSYS_MSG_TYPE_GET;
  }

  cclient_type_set(shell->cclient, type);

  process_more(shell_config_func, shell->cclient, argc, argv);
}

/* Short version of describe command. */
static void
shell_describe_short(struct vector *v) {
  uint32_t i;
  size_t len;
  /* Max item number in one line.  */
#define SHORT_ITEM_MAX         5
  size_t max[SHORT_ITEM_MAX] = {0, 0, 0, 0, 0};
  struct cnode *cnode;

  /* Check max string length. */
  for (i = 0; i < vector_max(v); i++) {
    if ((cnode = vector_slot(v, i)) != NULL) {
      len = strlen(cnode->name);
      if (max[i % SHORT_ITEM_MAX] < len) {
        max[i % SHORT_ITEM_MAX] = len;
      }
    }
  }

  /* Display short help. */
  printf("\n");
  for (i = 0; i < vector_max(v); i++) {
    if ((cnode = vector_slot(v, i)) != NULL) {
      printf ("%-*s", max[i % SHORT_ITEM_MAX] + 2, cnode->name);
      if ((i % SHORT_ITEM_MAX) == (SHORT_ITEM_MAX - 1)) {
        printf("\n");
      }
    }
  }

  /* Check new line is just printed out. */
  if (((i - 1) % SHORT_ITEM_MAX) != (SHORT_ITEM_MAX - 1)) {
    printf("\n");
  }
}

/* Long version of describe command. */
static void
shell_describe_long(struct vector *v, int exec) {
  struct cnode *cn;
  uint32_t i;
  char multi;

  printf("\nPossible completions:\n");
  if (exec) {
    printf("  <Enter>        Execute the current command\n");
  }
  for (i = 0; i < vector_max(v); i++) {
    cn = vector_slot(v, i);

    if (CHECK_FLAG(cn->flags, CNODE_FLAG_MULTI)) {
      multi = '+';
    } else {
      multi = ' ';
    }

    if (cn->guide == NULL && cn->type == CNODE_TYPE_RANGE) {
      printf("<%lld-%lld>", cn->min, cn->max);
    } else {
      const char *name;

      if (cn->guide != NULL) {
        name = cn->guide;
      } else {
        name = cn->name;
      }
      if (strlen(name) > 14) {
        printf("%c %-14s\n               ", multi, name);
      } else {
        printf("%c %-14s", multi, name);
      }
    }
    printf(" %s\n", cn->help ? cn->help : "");
  }
  printf("\n");
}

/* Describe wrapper. */
static int
shell_describe_func(void *val, int argc, char **argv) {
  struct vector *candidate;
  int exec;

  if (0) {
    printf("Ignoring %p\n", argv);
  }

  candidate = (struct vector *)val;
  exec = argc;

  shell_describe_long(candidate, exec);

  return 0;
}

/* Shell describe more. */
static void
shell_describe_more(struct vector *v, int exec) {
  process_more(shell_describe_func, v, exec, NULL);
}

/* Describe the commands. */
void
shell_describe(char *line) {
  enum cparse_result result;
  struct cnode *top;
  struct cparam *param;
  struct vector *matched;
  struct vector *candidate;

  param = cparam_alloc();
  top = shell_mode();
  result = cparse(line, top, param, 1);

  matched = param->matched;
  candidate = param->candidate;

  switch (result) {
    case CPARSE_EMPTY_LINE:
      shell_describe_long(candidate, 0);
      break;
    case CPARSE_AMBIGUOUS:
      if (param->tail) {
        printf("\n\n  Ambiguous command: [%s]\n",
               (char *)vector_slot(param->args, param->index));
      }
      shell_describe_long(matched, 0);
      break;
    case CPARSE_SUCCESS:
    case CPARSE_INCOMPLETE:
      if (param->tail) {
        shell_describe_long(candidate, result == CPARSE_SUCCESS);
      } else {
        shell_describe_short(matched);
      }
      break;
    case CPARSE_NO_MATCH:
      printf("\n\n");
      printf("  Invalid command: [%s]\n",
             (char *)vector_slot(param->args, param->index));
      printf("\n");
    default:
      break;
  }

  cparam_free(param);
}

/* Return matched string as char **. */
static char **
shell_completion_matches(struct vector *v) {
  uint32_t i;
  uint32_t j = 0;
  struct cnode *cnode;
  char **matches;

  for (i = 0; i < vector_max(v); i++) {
    cnode = vector_slot(v, i);
    if (cnode->type == CNODE_TYPE_KEYWORD) {
      j++;
    }
  }

  if (j == 0) {
    return NULL;
  }

  matches = calloc(1, sizeof(char *) * (j + 1));

  for (i = 0, j = 0; i < vector_max(v); i++) {
    cnode = vector_slot(v, i);
    if (cnode->type == CNODE_TYPE_KEYWORD) {
      matches[j++] = strdup(cnode->name);
    }
  }
  matches[j] = NULL;

  return matches;
}

/* Shell completion. */
char **
shell_completion(char *line, int *describe_status) {
  enum cparse_result result;
  char **matches = NULL;
  struct cparam *param;
  struct cnode *top;
  struct cnode *exec;
  struct vector *matched;
  struct vector *candidate;

  param = cparam_alloc();
  top = shell_mode();
  result = cparse(line, top, param, 1);

  matched = param->matched;
  candidate = param->candidate;

  *describe_status = 0;

  switch (result) {
    case CPARSE_EMPTY_LINE:
      *describe_status = 1;
      shell_describe_more(candidate, 0);
      break;
    case CPARSE_AMBIGUOUS:
      matches = shell_completion_matches(matched);
      break;
    case CPARSE_INCOMPLETE:
    case CPARSE_SUCCESS:
      exec = param->exec;
      if (param->tail) {
        if (vector_max(exec->v) == 1) {
          matches = shell_completion_matches(exec->v);
        } else {
          *describe_status = 1;
          shell_describe_more(candidate, result == CPARSE_SUCCESS);
        }
      } else {
        matches = shell_completion_matches(matched);
      }
      break;
    case CPARSE_NO_MATCH:
      printf("\n\n");
      printf("  Invalid command: [%s]\n",
             (char *)vector_slot(param->args, param->index));
      printf("\n");
      *describe_status = 1;
      break;
    default:
      break;
  }

  cparam_free(param);

  return matches;
}

/* Parse command line input. */
void
shell_exec(char *line) {
  enum cparse_result result;
  struct cnode *exec;
  struct cparam *param;
  struct confsys confsys;
  struct vector *argv;
  struct vector *matched;

  /* Parse the line. */
  param = cparam_alloc();

  /* Call cparse() with exec flat set to 1. */
  result = cparse(line, shell_mode(), param, 1);

  matched = param->matched;

  switch (result) {
    case CPARSE_NO_MATCH:
      printf("\n");
      printf("  Invalid command: [%s]\n",
             (char *)vector_slot(param->args, param->index));
      printf("\n");
      break;
    case CPARSE_SUCCESS:
      exec = param->exec;
      argv = param->argv;

      if (exec->callback) {
        (*exec->callback)(&confsys, (int)argv->max, (const char **)argv->index);
      } else {
        shell_config_more(param, (int)argv->max, (char **)argv->index);
      }
      break;
    case CPARSE_INCOMPLETE:
      printf("\n");
      printf("  Incomplete command: [%s]\n",
             (char *)vector_slot(param->args, param->index));
      printf("\n");
      break;
    case CPARSE_AMBIGUOUS:
      printf("\n  Ambiguous command: [%s]\n",
             (char *)vector_slot(param->args, param->index));
      shell_describe_long(matched, 0);
    default:
      break;
  }

#if 0
  if (cclient_type_get(shell->cclient) == CONFSYS_MSG_TYPE_SET) {
    while (1) {
      ;
    }
  }
#endif

  cparam_free(param);
}

/* Current top node of the mode. */
struct cnode *
shell_mode(void) {
  struct cnode *mode = NULL;

  switch (shell->mode) {
    case OPERATIONAL_MODE:
      mode = shell->op_mode;
      break;
    case CONFIGURATION_MODE:
      mode = shell->conf_mode;
      break;
    default:
      mode = shell->op_mode;
      break;
  }
  return mode;
}

/* Initialize shell. */
void
shell_init(void) {
  /* Allocate shell memory. */
  shell = (struct shell *)calloc(1, sizeof(struct shell));
  if (shell == NULL) {
    fprintf(stderr, "Shell memory allocation failed\n");
    exit(1);
  }

  /* Config system client. */
  shell->cclient = cclient_alloc();
  if (shell->cclient == NULL) {
    fprintf(stderr, "Shell memory allocation failed\n");
    exit(1);
  }

  /* Start from operational mode. */
  shell->mode = OPERATIONAL_MODE;

  /* Readline and signal init. */
  readline_init();

  /* Signals init. */
  signal_init();

  /* Load command line definition. */
  command_init();
}
