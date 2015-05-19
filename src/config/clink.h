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


#ifndef SRC_CONFIG_CLINK_H_
#define SRC_CONFIG_CLINK_H_

struct event;
struct cserver;

/* Config client link. */
struct clink {
  TAILQ_ENTRY(clink) entry;

  /* Socket for client communication. */
  int sock;

  /* Packet buffer. */
#define CLINK_PBUF_SIZE   4096
  struct pbuf *pbuf;

  /* Packet buffer list. */
  struct pbuf_list *pbuf_list;

  /* Parent of this clink. */
  struct cserver *cserver;

  /* Read and write event. */
  struct event *read_ev;
  struct event *write_ev;

  /* Is this show command? */
  int is_show;

  /* Lock. */
  int lock;
};

struct clink *
clink_alloc(void);

void
clink_free(struct clink *clink);

struct clink *
clink_get(struct cserver *cserver);

void
clink_start(struct clink *clink, int sock, struct cserver *cserver);

void
clink_stop(struct clink *clink);

int
clink_lock(struct clink *clink);

int
clink_unlock(struct clink *clink);

#endif /* SRC_CONFIG_CLINK_H_ */
