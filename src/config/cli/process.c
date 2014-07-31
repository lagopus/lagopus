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
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "process.h"
#include "signals.h"

struct process *process_list = NULL;

int external_command = 0;

/* When WAIT_ANY is defined, use it.  */
#ifdef WAIT_ANY
#define CHILD_PID         WAIT_ANY
#else
#define CHILD_PID         -1
#endif /* WAIT_ANY. */

/* Define default pager.  */
const char *pager = "more";

/* Allocate a new process for pipeline. */
static struct process *
pipe_add(struct process *prev) {
  struct process *proc;

  proc = calloc(1, sizeof(struct process));
  if (proc == NULL) {
    return NULL;
  }

  if (prev) {
    prev->next = proc;
  }

  return proc;
}

/* Free pipeline memory.  */
static void
pipe_free(struct process *proc) {
  struct process *next;

  while (proc) {
    next = proc->next;
    free(proc);
    proc = next;
  }
}

/* Exec child process.  */
static int
exec_process(struct process *p, int in_fd, int out_fd, int err_fd) {
  int ret = 0;

  /* Set the handler to the default.  */
  signal_set(SIGINT,  SIG_DFL);
  signal_set(SIGQUIT, SIG_DFL);
  signal_set(SIGCHLD, SIG_DFL);
  signal_set(SIGALRM, SIG_DFL);
  signal_set(SIGPIPE, SIG_DFL);
  signal_set(SIGUSR1, SIG_DFL);

  /* No job control.  */
  signal_set(SIGTSTP, SIG_IGN);
  signal_set(SIGTTIN, SIG_IGN);
  signal_set(SIGTTOU, SIG_IGN);

  /* Standard input, output and error.  */
  if (in_fd != STDIN_FILENO) {
    if (dup2(in_fd, STDIN_FILENO) < 0) {
      perror("dup2(in_fd)");
      exit(1);
    }
    close(in_fd);
  }
  if (out_fd != STDOUT_FILENO) {
    if (dup2(out_fd, STDOUT_FILENO) < 0) {
      perror("dup2(out_fd)");
      exit(1);
    }
    close(out_fd);
  }
  if (err_fd != STDERR_FILENO) {
    if (dup2(err_fd, STDERR_FILENO) < 0) {
      perror("dup2(err_fd)");
      exit(1);
    }
    close(err_fd);
  }

  /* Internal function. */
  if (p->internal) {
    (*(p->func))(p->func_val, p->func_argc, p->func_argv);
    exit(0);
  }

  /* Exec. */
  ret = execvp(p->argv[0], (char **const)p->argv);

  /* Should not be here unless error occurred.  */
  perror ("execvp");
  exit (1);

  return ret;
}

/* Mark process stautus.  */
static int
process_update_status(pid_t pid, int status) {
  struct process *p;

  if (pid > 0) {
    for (p = process_list; p; p = p->next) {
      if (p->pid == pid) {
        p->status = status;

        if (! WIFSTOPPED(status)) {
          p->completed = 1;
        }
        return 0;
      }
    }
    fprintf(stderr, "No child process %d\n", pid);

    return -1;
  } else if (pid == 0 || errno == ECHILD) {
    return -1;
  } else {
    perror("waitpid");
    return -1;
  }
}

/* Return 1 when all child processes are completed.  */
static int
process_completed(void) {
  struct process *p;

  for (p = process_list; p; p = p->next) {
    if (! p->completed) {
      return 0;
    }
  }
  return 1;
}

/* Wait all of child processes.  Clear process list after all of child
   process is finished.  */
void
process_wait_for(void) {
  pid_t pid;
  int status;

  do {
    pid = waitpid(CHILD_PID, &status, WUNTRACED);
  } while (process_update_status(pid, status) == 0 &&
           process_completed() == 0);

  pipe_free(process_list);
  process_list = NULL;

  if (external_command) {
    external_command = 0;
  }
}

static void
pipe_line(struct process *process) {
  int in_fd;
  int out_fd;
  int err_fd;
  int pipe_fd[2];
  int ret;
  pid_t pid;
  struct process *p;

  in_fd = STDIN_FILENO;
  err_fd = STDERR_FILENO;
  process_list = process;

  for (p = process; p; p = p->next) {
    /* Prepare pipe. */
    if (p->next) {
      ret = pipe(pipe_fd);
      if (ret < 0) {
        perror("pipe");
        return;
      }
      out_fd = pipe_fd[1];
    } else {
      /* Redirection of output. */
      if (p->path) {
        out_fd = open(p->path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (out_fd < 0) {
          fprintf(stderr, "%% Can't open output file %s\n", p->path);
          return;
        }
      } else {
        out_fd = STDOUT_FILENO;
      }
    }

    /* Fork. */
    pid = fork();
    if (pid < 0) {
      perror("fork");
      return;
    }
    if (pid == 0) {
      /* Pipe's incoming fd is not used by child process. */
      if (p->next) {
        close(pipe_fd[0]);
      }

      /* This is child process. */
      ret = exec_process(p, in_fd, out_fd, err_fd);
      if (ret < 0) {
        return;
      }
    } else {
      /* This is parent process. */
      p->pid = pid;
    }

    if (in_fd != STDIN_FILENO) {
      close(in_fd);
    }
    if (out_fd != STDOUT_FILENO) {
      close(out_fd);
    }
    in_fd = pipe_fd[0];
  }

  /* Wait for all of child processes. */
  process_wait_for();
}

/* Execute command in child process. */
int
process_exec(const char *command, ...) {
  va_list args;
  unsigned int argc;
  const char **argv;
  struct process *proc;

  /* Count arguments. */
  va_start(args, command);
  for (argc = 1; va_arg(args, char *) != NULL; ++argc) {
    continue;
  }
  va_end(args);

  /* Malloc argv. Add 1 for NULL termination in last argv. */
  argv = (const char **)calloc(1, sizeof(char *) * (argc + 1));
  if (! argv) {
    return -1;
  }

  /* Copy arguments to argv. */
  argv[0] = command;

  va_start(args, command);
  for (argc = 1; (argv[argc] = va_arg(args, char *)) != NULL; ++argc) {
    continue;
  }
  va_end(args);

  proc = pipe_add(NULL);
  if (! proc) {
    return -1;
  }
  proc->argv = argv;

  /* Execute command. */
  external_command = 1;
  pipe_line(proc);

  /* Free argv. */
  free(argv);

  return 0;
}

/* Set internal func process.  */
static struct process *
process_internal_pipe (INTERNAL_FUNC func, void *val, int argc, char **argv) {
  struct process *proc;

  proc = pipe_add (NULL);
  if (! proc) {
    return NULL;
  }

  proc->func = func;
  proc->func_val = val;
  proc->func_argc = argc;
  proc->func_argv = argv;
  proc->internal = 1;

  return proc;
}

/* Pipe to more. */
int
process_more(INTERNAL_FUNC func, void *arg, int argc, char **argv) {
  struct process *proc_top;
  struct process *proc;
  const char *argv_more[2];

  /* Initialize more array. */
  argv_more[0] = pager;
  argv_more[1] = NULL;

  /* Internal command.  */
  proc_top = proc = process_internal_pipe(func, arg, argc, argv);
  if (! proc_top) {
    return -1;
  }

  /* More */
  proc = pipe_add(proc);
  if (! proc) {
    return -1;
  }
  proc->argv = argv_more;

  pipe_line(proc_top);

  return 0;
}
