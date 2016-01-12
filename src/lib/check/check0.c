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


static inline const char *
myname(const char *argv0) {
  const char *p = (const char *)strrchr(argv0, '/');
  if (p != NULL) {
    p++;
  } else {
    p = argv0;
  }
  return p;
}


int
main(int argc, const char *const argv[]) {
  const char *nm = myname(argv[0]);
  (void)argc;

  lagopus_log_set_trace_flags(TRACE_OFPT_HELLO |
                              TRACE_OFPT_ERROR |
                              TRACE_OFPT_METER_MOD);

  lagopus_msg("this should emitted to stderr.\n");

  lagopus_msg_trace(TRACE_OFPT_HELLO, false, "hello test.\n");
  lagopus_msg_trace(TRACE_OFPT_ERROR, false, "error test.\n");
  lagopus_msg_trace(TRACE_OFPT_HELLO |
                    TRACE_OFPT_ERROR,
                    false, "hello|error test.\n");

  /*
   * log to stderr.
   */
  if (IS_LAGOPUS_RESULT_OK(
        lagopus_log_initialize(LAGOPUS_LOG_EMIT_TO_UNKNOWN, NULL,
                               false, true, 1)) == false) {
    lagopus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  lagopus_dprint("debug to stderr.\n");

  /*
   * log to file.
   */
  if (IS_LAGOPUS_RESULT_OK(
        lagopus_log_initialize(LAGOPUS_LOG_EMIT_TO_FILE, "./testlog.txt",
                               false, true, 10)) == false) {
    lagopus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  lagopus_dprint("debug to file.\n");
  lagopus_msg_debug(5, "debug to file, again.\n");
  lagopus_msg_trace(TRACE_OFPT_HELLO, false, "hello file test.\n");
  lagopus_msg_trace(TRACE_OFPT_ERROR, false, "error file test.\n");
  lagopus_msg_trace(TRACE_OFPT_HELLO |
                    TRACE_OFPT_ERROR,
                    false, "hello|error file test.\n");

  if (IS_LAGOPUS_RESULT_OK(
        lagopus_log_initialize(LAGOPUS_LOG_EMIT_TO_SYSLOG, nm,
                               false, false, 10)) == false) {
    lagopus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  lagopus_msg_debug(5, "debug to syslog.\n");
  lagopus_msg_trace(TRACE_OFPT_HELLO, false, "hello syslog test.\n");
  lagopus_msg_trace(TRACE_OFPT_ERROR, false, "error syslog test.\n");
  lagopus_msg_trace(TRACE_OFPT_HELLO |
                    TRACE_OFPT_ERROR,
                    false, "hello|error syslog test.\n");

  /*
   * log to stderr, again.
   */
  if (IS_LAGOPUS_RESULT_OK(
        lagopus_log_initialize(LAGOPUS_LOG_EMIT_TO_UNKNOWN, NULL,
                               false, true, 1)) == false) {
    lagopus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  lagopus_dprint("will exit 1 ...\n");
  lagopus_exit_error(1, "exit 1 on purpose.\n");

  return 0;
}
