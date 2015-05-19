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
#include <stdint.h>
#include <string.h>

#include "confsys.h"
#include "cclient.h"

#include "command.h"
#include "shell.h"
#include "process.h"
#include "readline.h"

/* Local commands. */
CALLBACK(configure_func) {
  ARG_USED();
  printf("Entering configuration mode\n");
  shell->mode = CONFIGURATION_MODE;
  return 0;
}

CALLBACK(exit_func) {
  ARG_USED();
  exit(0);
  return 0;
}

CALLBACK(start_shell_func) {
  ARG_USED();
  process_exec("bash", NULL);
  return 0;
}

CALLBACK(ping_func) {
  ARG_USED();
  process_exec("ping", argv[1], NULL);
  return 0;
}

CALLBACK(ping6_func) {
  ARG_USED();
  process_exec("ping6", argv[1], NULL);
  return 0;
}

CALLBACK(telnet_func) {
  ARG_USED();
  process_exec("telnet", argv[1], NULL);
  return 0;
}

CALLBACK(ssh_func) {
  ARG_USED();
  process_exec("ssh", argv[1], NULL);
  return 0;
}

CALLBACK(history_func) {
  ARG_USED();
  readline_history();
  return 0;
}

CALLBACK(mode_exit_func) {
  ARG_USED();
  shell->mode = OPERATIONAL_MODE;
  return 0;
}

CALLBACK(confsys_exec_func) {
  ARG_USED();
  cclient_type_set(shell->cclient, CONFSYS_MSG_TYPE_EXEC);
  process_more(shell_config_func, shell->cclient, argc, argv);
  return CONFIG_SUCCESS;
}

void
command_init(void) {
  struct cnode *cnode;

  /* Load schema. */
  shell->op_mode = confsys_schema_load("operational.xml");
  shell->conf_mode = confsys_schema_load("configuration.xml");
  shell->config = cnode_alloc();

  /* Something wrong. */
  if (shell->conf_mode == NULL || shell->op_mode == NULL) {
    fprintf(stderr, "Schema file read error exiting.\n");
    exit(1);
  }

  /* Install operational command. */
  cnode = shell->op_mode;
  install_callback(cnode, "start shell", start_shell_func);
  install_callback(cnode, "configure", configure_func);
  install_callback(cnode, "history", history_func);
  install_callback(cnode, "ping WORD", ping_func);
  install_callback(cnode, "ping A.B.C.D", ping_func);
  install_callback(cnode, "ping X:X::X:X", ping6_func);
  install_callback(cnode, "telnet WORD", telnet_func);
  install_callback(cnode, "telnet A.B.C.D", telnet_func);
  install_callback(cnode, "telnet X:X::X:X", telnet_func);
  install_callback(cnode, "ssh WORD", telnet_func);
  install_callback(cnode, "ssh A.B.C.D", telnet_func);
  install_callback(cnode, "ssh X:X::X:X", telnet_func);
  install_callback(cnode, "exit", exit_func);
  install_callback(cnode, "quit", exit_func);

  /* Install configuration command. */
  cnode = shell->conf_mode;
  install_callback(cnode, "exit", mode_exit_func);
  install_callback(cnode, "show", confsys_exec_func);
  install_callback(cnode, "save", confsys_exec_func);
  install_callback(cnode, "stop-process", confsys_exec_func);

  /* Link "set" node to "delete" node. */
  cnode_link(cnode, "set", "delete");

  /* Mark set node to distinguish set and delete. */
  cnode = cnode_lookup(shell->conf_mode, "set");
  if (cnode) {
    cnode_set_flag(cnode, CNODE_FLAG_SET_NODE);
  }
  cnode = cnode_lookup(shell->conf_mode, "delete");
  if (cnode) {
    cnode_set_flag(cnode, CNODE_FLAG_DELETE_NODE);
  }
}
