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
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "signals.h"
#include "process.h"
#include "shell.h"
#include "confsys.h"
#include "cclient.h"

/* For sigsetjmp() & siglongjmp(). */
extern sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call. */
extern int jmpflag;

/* Signal set function declaration. */
RETSIGTYPE *signal_set(int, void(*)(int));

/* SIGINT handler.  This function takes care of ^C input. */
static void
sigint(int signo) {
  /* Debug */
  if (0) {
    printf("signo %d\n", signo);
  }

  /* Reinstall signal. */
  signal_set(SIGINT, sigint);

  /* Wait child process. */
  if (process_list) {
    process_wait_for();
  } else {
    printf("\n");
  }

  /* Make it sure cclient is reset. */
  cclient_stop(shell->cclient);

  /* Long jump flag check. */
  if (! jmpflag) {
    return;
  }

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp(jmpbuf, 1);
}

/* SIGTSTP handler.  This function takes care of ^Z input. */
static void
sigtstp(int signo) {
  /* Debug */
  if (0) {
    printf("signo %d\n", signo);
  }

  /* Reinstall signal. */
  signal_set(SIGTSTP, sigtstp);

  /* Do nothing when sub-process is running. */
  if (process_list) {
    return;
  }

  /* New line. */
  printf("\n");

  /* Make it sure cclient is reset. */
  cclient_stop(shell->cclient);

  /* Long jump flag check. */
  if (! jmpflag) {
    return;
  }

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp(jmpbuf, 1);
}

/* Time out handler. */
static void
sigalarm(int signo) {
  unsigned int idle = 0;
  unsigned int timeout = 0;

  /* Debug */
  if (0) {
    printf("signo %d\n", signo);
  }

  /* Get tty idle value and time out value. */
  /* idle = ttyidle(); */
  /* timeout = timeout_get(); */

  if (idle < timeout) {
    /* Reset time out. */
    alarm(timeout - idle);
  } else {
    /* Time out. */
    printf("\nconnection is timed out.\n");
    exit(0);
  }
}

/* Notification of exit. */
static void
sigterm(int signo) {
  /* Debug */
  if (0) {
    printf("signo %d\n", signo);
  }

  /* Exit shell. */
  printf("\nConnection is closed by administrator!\n");

  exit(0);
}

/* Get terminal windows size. */
static int
terminal_winsize_get(int sock, unsigned short *row, unsigned short *col) {
  int ret;
  struct winsize w;

  ret = ioctl(sock, TIOCGWINSZ, &w);
  if (ret < 0) {
    return -1;
  }

  *row = w.ws_row;
  *col = w.ws_col;

  return 0;
}

#if 0
/* Set terminal windows size. */
static int
terminal_winsize_set(int sock, unsigned short row, unsigned short col) {
  int ret;
  struct winsize w;

  w.ws_row = row;
  w.ws_col = col;

  ret = ioctl(sock, TIOCSWINSZ, &w);
  if (ret < 0) {
    return -1;
  }

  return 0;
}
#endif

/* Terminal window size change. */
static void
sigwinch(int signo) {
  unsigned short row;

  /* Debug */
  if (0) {
    printf("signo %d\n", signo);
  }

  terminal_winsize_get(STDIN_FILENO, &row, &shell->win_col);

  if (row != shell->win_row) {
    shell->win_row_orig = row;
#if 0 /* Disable until we have terminal row setting command. */
    terminal_winsize_set(STDIN_FILENO, shell->win_row, shell->win_col);
#endif
  }
}

/* Set signal handler. */
RETSIGTYPE *
signal_set(int signo, void(*func)(int)) {
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags = 0;
  sig.sa_flags |= SA_RESTART;

  ret = sigaction(signo, &sig, &osig);

  if (ret < 0) {
    return (SIG_ERR);
  } else {
    return (osig.sa_handler);
  }
}

/* Initialize signal handlers. */
void
signal_init(void) {
  signal_set(SIGINT,   sigint);
  signal_set(SIGTSTP,  sigtstp);
  signal_set(SIGALRM,  sigalarm);
  signal_set(SIGPIPE,  SIG_IGN);
  signal_set(SIGQUIT,  SIG_IGN);
  signal_set(SIGTTIN,  SIG_IGN);
  signal_set(SIGTTOU,  SIG_IGN);
  signal_set(SIGTERM,  sigterm);
  signal_set(SIGHUP,   SIG_IGN);
  signal_set(SIGWINCH, sigwinch);
}
