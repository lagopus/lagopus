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


/* Time utility functions. */
#include "lagopus_config.h"

#include <sys/time.h>
#include "timeutil.h"

struct timeval timeval_adjust(struct timeval a);

/* Compare two timeval.  Return positive value when a > b, zero when a
 * == b, negative when a < b. */
long int
timeval_cmp (struct timeval a, struct timeval b) {
  return (a.tv_sec == b.tv_sec ?
          a.tv_usec - b.tv_usec : a.tv_sec - b.tv_sec);
}

/* Adjust negative value of the tv_usec. */
struct timeval
timeval_adjust(struct timeval a) {
  while (a.tv_usec >= TV_USEC_PER_SEC) {
    a.tv_usec -= TV_USEC_PER_SEC;
    a.tv_sec++;
  }

  while (a.tv_usec < 0) {
    a.tv_usec += TV_USEC_PER_SEC;
    a.tv_sec--;
  }

  if (a.tv_sec > TV_USEC_PER_SEC) {
    a.tv_sec = TV_USEC_PER_SEC;
  }

  return a;
}

/* Subtract b from a. */
struct timeval
timeval_subtract(struct timeval a, struct timeval b) {
  struct timeval val;

  val.tv_usec = a.tv_usec - b.tv_usec;
  val.tv_sec = a.tv_sec - b.tv_sec;

  return timeval_adjust (val);
}
