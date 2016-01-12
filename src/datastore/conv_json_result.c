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

#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "lagopus_dstring.h"
#include "conv_json_result.h"

struct datastore_json_result {
  lagopus_result_t type;
  lagopus_dstring_t msg;
};

static const char *const result_strs[] = {
  "OK",                                 /*  0 */
  "ANY_FAILURES",                       /*  1 */
  "POSIX_API_ERROR",                    /*  2 */
  "NO_MEMORY",                          /*  3 */
  "NOT_FOUND",                          /*  4 */
  "ALREADY_EXISTS",                     /*  5 */
  "NOT_OPERATIONAL",                    /*  6 */
  "INVALID_ARGS",                       /*  7 */
  "NOT_OWNER",                          /*  8 */
  "NOT_STARTED",                        /*  9 */
  "TIMEDOUT",                           /* 10 */
  "ITERATION_HALTED",                   /* 11 */
  "OUT_OF_RANGE",                       /* 12 */
  "NAN",                                /* 13 */
  "OFP_ERROR",                          /* 14 */
  "ALREADY_HALTED",                     /* 15 */
  "INVALID_OBJECT",                     /* 16 */
  "CRITICAL_REGION_NOT_CLOSED",         /* 17 */
  "CRITICAL_REGION_NOT_OPENED",         /* 18 */
  "INVALID_STATE_TRANSITION",           /* 19 */
  "SOCKET_ERROR",                       /* 20 */
  "BUSY",                               /* 21 */
  "STOP",                               /* 22 */
  "SNMP_API_ERROR",                     /* 23 */
  "TLS_CONN_ERROR",                     /* 24 */
  "EINPROGRESS",                        /* 25 */
  "OXM_ERROR",                          /* 26 */
  "UNSUPPORTED",                        /* 27 */
  "QUOTE_NOT_CLOSED",                   /* 28 */
  "NOT_ALLOWED",                        /* 29 */
  "NOT_DEFINED",                        /* 30 */
  "WAKEUP_REQUESTED",                   /* 31 */
  "TOO_MANY_OBJECTS",                   /* 32 */
  "DATASTORE_INTERP_ERROR",             /* 33 */
  "EOF",				/* 34 */
  "NO_MORE_ACTION",			/* 35 */
  "TOO_LARGE",				/* 36 */
  "TOO_SMALL",				/* 37 */
  "TOO_LONG",				/* 38 */
  "TOO_SHORT",				/* 39 */
  "ADDR_RESOLVER_FAILURE",		/* 40 */
  "OUTPUT_FAILURE",			/* 41 */
  "INVALID_STATE",			/* 42 */
  "INVALID_NAMESPACE",			/* 43 */
  NULL
};

static ssize_t result_strs_num =
  sizeof(result_strs) / sizeof(const char *);

const char *
datastore_json_result_string_get(lagopus_result_t ret) {
  if (ret < 0) {
    ssize_t i = -ret;
    if (i < result_strs_num) {
      return result_strs[i];
    } else {
      return "??? result string out of range ???";
    }
  } else {
    return result_strs[0];
  }
}
