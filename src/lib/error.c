/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
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





static const char *const s_error_strs[] = {
  "No error(s)",			/*  0 */
  "Unknown, any failure(s)",		/*  1 */
  "POSIX API error(s)",			/*  2 */
  "Insufficient memory",		/*  3 */
  "Not found",				/*  4 */
  "Already exists",			/*  5 */
  "Not operational at this moment",	/*  6 */
  "Invalid argument(s)",		/*  7 */
  "Not the owner",			/*  8 */
  "Not started",			/*  9 */
  "Timedout",				/* 10 */
  "Iteration halted",			/* 11 */
  "Out of range",			/* 12 */
  "Not a number",			/* 13 */
  "OpenFlow protocol error(s)",		/* 14 */
  "Already halted",			/* 15 */
  "An invalid object",			/* 16 */
  "A critical region is not closed",	/* 17 */
  "A critical region is not opened",	/* 18 */
  "Invalid state transition",		/* 19 */
  "Socket error(s)",			/* 20 */
  "Busy",				/* 21 */
  "Stop",				/* 22 */
  "Net-SNMP error(s)",			/* 23 */
  "TLS connection error(s)",		/* 24 */
  "EINPROGRESS error(s)",		/* 25 */
  "OXM error(s)",			/* 26 */
  "Unsupported",			/* 27 */
  "Quote not closed",			/* 28 */
  "Not allowed",			/* 29 */
  "Not defined",			/* 30 */
  "Wake up requested",			/* 31 */
  "Too many objects",			/* 32 */
  "Datastore interp error(s)",		/* 33 */
  "End of file",			/* 34 */
  "No more action",			/* 35 */
  "Too large",				/* 36 */
  "Too small",				/* 37 */
  "Too long",				/* 38 */
  "Too short",				/* 39 */
  "Address resolver failure",		/* 40 */
  "Output failure",			/* 41 */
  "Invalid state",			/* 42 */
  "Invalid namespace",			/* 43 */
  "Interrupted",			/* 44 */

  NULL
};


static ssize_t s_n_error_strs =
  sizeof(s_error_strs) / sizeof(const char *);





const char *
lagopus_error_get_string(lagopus_result_t r) {
  if (r < 0) {
    ssize_t ar = -r;
    if (ar < s_n_error_strs) {
      return s_error_strs[ar];
    } else {
      return "??? error string out of range ???";
    }
  } else if (r == 0) {
    return s_error_strs[0];
  } else {
    return "Greater than zero, means not errors??";
  }
}

