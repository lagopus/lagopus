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


#


static void
s_block_forever(void) {
  do {
    (void)select(0, NULL, NULL, NULL, NULL);
  } while (true);
}


static void
s_lag_sighandler(int sig) {
  (void)sig;
  lagopus_msg_info("called.\n");
}


static void
s_posix_sighandler(int sig) {
  (void)sig;
  lagopus_msg_info("called.\n");
}


static void
s_signal_quit(int sig) {
  (void)sig;
  lagopus_msg_info("\n\ncalled. bye.\n\n");
  exit(0);
}





int
main(int argc, const char *const argv[]) {
  pid_t pid;
  (void)argv;

  (void)lagopus_signal(SIGHUP, s_lag_sighandler, NULL);
  (void)lagopus_signal(SIGINT, s_signal_quit, NULL);

  (void)signal(SIGHUP, s_posix_sighandler);
  (void)signal(SIGINT, s_signal_quit);

  if (argc > 1) {
    lagopus_signal_old_school_semantics();
  }

  pid = fork();
  if (pid == 0) {
    s_block_forever();
  } else if (pid > 0) {
    int st;
    (void)waitpid(pid, &st, 0);
    return 0;
  } else {
    perror("fork");
    return 1;
  }
}
