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


#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "clex.h"

/* Macro. */
#define WHITE_SPACE(C)     ((C) == ' ' || (C) == '\t' || (C) == '\n')
#define EOL(C)             ((C) == '\0')

/* Lexical analysis of the command line.  It convert space delimiter
 * command into vector.  */
void
clex(char *line, struct vector *args) {
  char *rem;
  char *cp;

  rem = cp = strdup(line);

  /* Initialize. */
  vector_reset(args);

  /* No input. */
  if (cp == NULL) {
    goto out;
  }

  /* Skip white spaces. */
  while (WHITE_SPACE(*cp)) {
    cp++;
  }

  /* End of line? */
  if (EOL(*cp)) {
    goto out;
  }

  /* Parse until end of line. */
  for (;;) {
    /* Starting pointer. */
    char c;
    char *sp;

    /* Beginning of the word. */
    sp = cp;

    /* Skip word. */
    while (!WHITE_SPACE(*cp) && !EOL(*cp)) {
      cp++;
    }

    /* Remember cp. */
    c = *cp;

    /* Set current word to vector. */
    *cp = '\0';
    vector_set(args, strdup(sp));

    /* Revert cp. */
    *cp = c;

    /* End of line. */
    if (EOL(*cp)) {
      goto out;
    }

    /* Make white space to be word separator. */
    while (WHITE_SPACE(*cp)) {
      cp++;
    }
  }

out:
  free(rem);
}

