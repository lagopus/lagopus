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


enum shell_mode {
  OPERATIONAL_MODE,
  CONFIGURATION_MODE
};

enum shell_flags {
  SHELL_FLAG_DESCRIBE_FLIP     = (1 << 0),
};

struct cclient;

struct shell {
  /* Input line. */
  char *line;

  /* Current shell mode. */
  enum shell_mode mode;

  /* Commands for operational mode. */
  struct cnode *op_mode;
  struct cnode *conf_mode;

  /* Configuration. */
  struct cnode *config;

  /* Flags. */
  uint32_t flags;

  /* Terminal size. */
  unsigned short win_col;
  unsigned short win_row;
  unsigned short win_row_orig;

  /* Config system client. */
  struct cclient *cclient;
};

/* #define MAXHOSTNAMELEN 255 */
#define MAXPROMPTLEN     3

void
shell_init(void);

struct cnode *
shell_mode(void);

void
shell_exec(char *line);

char **
shell_completion(char *line, int *describe_status);

void
shell_describe(char *);

int
shell_config_func(void *arg, int argc, char **argv);

/* Shell singleton. */
extern struct shell *shell;

