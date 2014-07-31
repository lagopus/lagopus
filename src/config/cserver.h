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


#ifndef SRC_CONFIG_CSERVER_H_
#define SRC_CONFIG_CSERVER_H_

#include "lagopus/queue.h"
#include "lagopus/pbuf.h"
#include "event.h"

/* Forward declaration. */
struct clink;

/* Clink list. */
TAILQ_HEAD(clink_list, clink);

/* Config server. */
struct cserver {
  /* Server socket. */
  int sock_conf;
  int sock_show;

  /* Event manager. */
  struct event_manager *em;

  /* Event for accept. */
  struct event *accept_ev;
  struct event *accept_show;

  /* clink list. */
  struct clink_list clink;
  struct clink_list unused;

  /* Show tree. */
  struct cnode *show;

  /* Exec tree. */
  struct cnode *exec;

  /* Schema trees. */
  struct cnode *schema;

  /* Config tree. */
  struct cnode *config;

  /* Lock. */
  int lock;
};

struct cserver *
cserver_alloc(void);

void
cserver_free(struct cserver *cserver);

int
cserver_start(struct cserver *cserver, struct event_manager *em);

void
cserver_install_callback(struct cserver *cserver);

void
clink_stop(struct clink *clink);

void
clink_process(struct clink *clink, enum confsys_msg_type type, char *str);

#endif /* SRC_CONFIG_CSERVER_H_ */
