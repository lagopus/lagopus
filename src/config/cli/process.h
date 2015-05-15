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


/* Internal function.  */
typedef int (*INTERNAL_FUNC) (void *, int, char **);

/* Process structure.  */
struct process {
  /* Single linked list.  */
  struct process *next;

  /* Execute argv.  */
  const char **argv;

  /* Process id of child process.  */
  pid_t pid;

  /* Status.  */
  int status;

  /* Completed.  */
  int completed;

  /* Internal function. */
  INTERNAL_FUNC func;

  /* Internal function value.  */
  void *func_val;

  /* Internal function argument count.  */
  int func_argc;

  /* Internal function arguments.  */
  char **func_argv;

  /* For redirect of output.  */
  char *path;

  /* Internal function flag. */
  int internal;
};

extern struct process *process_list;

int
process_exec(const char *command, ...);

int
process_more(INTERNAL_FUNC func, void *arg, int argc, char **argv);

void
process_wait_for(void);
