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
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include "readline.h"
#include "shell.h"
#include "lagopus_apis.h"

/* For longjmp to support sub-process kill procedure. */
sigjmp_buf jmpbuf;
int jmpflag = 0;

/* Usage function. */
static void
usage (char *progname, int status) {
  if (status) {
    fprintf(stderr, "Try `%s --help' for more information.\n", progname);
  } else {
    fprintf(stdout,
            "Usage : %s [OPTION...]\n\n"
            "lagosh -- Lagopus virtual switch shell.\n"
            "  -h, --help               Display this help and exit\n"
            "\n", progname);
  }
  exit (status);
}

/* Main routine. */
int
main(int argc, char *argv[]) {
  /* My name is... */
  char *p;
  char *progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* longopts structure. */
  struct option longopts[] = {
    { "help", no_argument, NULL, 'h'},
    { NULL,   no_argument, NULL, '\0' }
  };

  /* Make signal to be back to original status. */
  lagopus_signal_old_school_semantics();

  /* Option handling. */
  while (1) {
    int opt = getopt_long(argc, argv, "h", longopts, 0);

    if (opt == EOF) {
      break;
    }

    switch (opt) {
      case 0:
        break;
      case 'h':
        usage (progname, 0);
        break;
      default:
        usage (progname, 1);
        break;
    }
  }

  /* Initialization. */
  shell_init();

  /* Set jump. */
  sigsetjmp(jmpbuf, 1);
  jmpflag = 1;

  /* Read line and parse it. */
  while (readline_gets()) {
    shell_exec(shell->line);
  }

  /* New line. */
  printf("\n");

  /* Normal exit. */
  return 0;
}
