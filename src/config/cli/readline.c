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


#include "lagopus_apis.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/utsname.h>
#include "libedit/editline/readline.h"
#include "shell.h"
#include "readline.h"
#include "process.h"

/* Extern from libedit library. */
extern char *rl_line_buffer;

/* Shell prompt string. */
static const char *
shell_prompt(void) {
  char sign = '>';
  static char prompt_str[MAXHOSTNAMELEN + MAXPROMPTLEN + 1];
  struct utsname names;

  uname(&names);

  if (shell->mode == CONFIGURATION_MODE) {
    printf("[edit]\n");
    sign = '#';
  }

  snprintf(prompt_str, MAXHOSTNAMELEN + MAXPROMPTLEN,
           "%s%c ", names.nodename, sign);

  return prompt_str;
}

/* Read one line from libedit. */
char *
readline_gets(void) {
  while (1) {
    if (shell->line) {
      free(shell->line);
      shell->line = NULL;
    }

    shell->line = readline(shell_prompt());

    if (shell->line == NULL && shell->mode == CONFIGURATION_MODE) {
      printf("\nUse \"exit\" to leave configuration mode\n");
      continue;
    }

    if (shell->line && *(shell->line)) {
      add_history(shell->line);
    }

    return shell->line;
  }
}

/* Show readline history. */
void
readline_history(void) {
  history_show();
}

/* Result of complete() in order to put the space in correct
 * places. */
int completion_status = 0;

/* Describe status for reflesh the command line when. */
int describe_status = 0;

/* Completion match function. */
static char *
readline_completion_matches(const char *text, int state) {
  static char **matched = NULL;
  static int matched_index = 0;

  /* Ignoring argument. */
  if (0) {
    printf("Ignoring text %s", text);
  }

  /* Do not complete when we are not at the end of the line. */
  if (rl_point != rl_end) {
    return NULL;
  }

  /* First call. */
  if (! state) {
    /* Reset index when first time. */
    matched_index = 0;

    /* Execute completion only at the first call. */
    matched = shell_completion(rl_line_buffer, &describe_status);

    /* Update complete status. */
    if (matched && matched[0] && matched[1] == NULL) {
      completion_status = 1;
    } else {
      completion_status = 0;
    }
  }

  /* This function is called until this function returns NULL. */
  if (matched && matched[matched_index]) {
    return matched[matched_index++];
  }

  return NULL;
}

/* Glue function for readline_completion_matches.  */
static char **
readline_completion(char *cp, int start, int end) {
  /* Ignoring argument. */
  if (0) {
    printf("Ignoring start %d and end %d", start, end);
  }
  return completion_matches(cp, readline_completion_matches);
}

/* Disable readline's filename completion.  */
static int
readline_completion_empty(const char *ignore, int invoking_key) {
  /* Ignoring argument. */
  if (0) {
    printf("Ignoring char %c %d", *ignore, invoking_key);
  }
  return 0;
}

/* Internal fuction for more. */
static int
readline_describe_func(void *val, int argc, char **argv) {
  if (0) {
    printf("Ignoring %p %d %p", val, argc, argv);
  }

  /* Need put more for paging. */
  shell_describe(rl_line_buffer);

  return 0;
}

/* This function is directly called from libedit library. */
int
readline_describe(int ch) {
  /* Ignoring argument. */
  if (0) {
    printf("Ignoring readline_describe argument ch %d\n", ch);
  }

  /* Need put more for paging. */
  process_more(readline_describe_func, NULL, 0, NULL);

  return 0;
}

/* Readline init. */
void
readline_init(void) {
  /* Disable filename completion.  */
  rl_completion_entry_function = readline_completion_empty;

  /* Register completion function.  */
  rl_attempted_completion_function = (CPPFunction *)readline_completion;
}

