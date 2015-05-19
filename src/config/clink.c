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
#include "confsys.h"

/* Allocate clink which is connection to cclinet. */
struct clink *
clink_alloc(void) {
  struct clink *clink;

  clink = (struct clink *)calloc(1, sizeof(struct clink));
  if (clink == NULL) {
    return NULL;
  }

  clink->pbuf = pbuf_alloc(CLINK_PBUF_SIZE);
  if (clink->pbuf == NULL) {
    free(clink);
    return NULL;
  }

  clink->pbuf_list = pbuf_list_alloc();
  if (clink->pbuf_list == NULL) {
    pbuf_free(clink->pbuf);
    free(clink);
    return NULL;
  }

  clink->sock = -1;

  return clink;
}

/* Free clink. */
void
clink_free(struct clink *clink) {
  if (clink->sock >= 0) {
    close(clink->sock);
  }
  if (clink->pbuf) {
    pbuf_free(clink->pbuf);
  }
  if (clink->pbuf_list) {
    pbuf_list_free(clink->pbuf_list);
  }
  free(clink);
}

/* Get clink from unsed list, otherwise allocate a new one. */
struct clink *
clink_get(struct cserver *cserver) {
  struct clink *clink;

  clink = TAILQ_FIRST(&cserver->unused);
  if (!clink) {
    clink = clink_alloc();
  } else {
    TAILQ_REMOVE(&cserver->unused, clink, entry);
  }
  return clink;
}

/* Lock the config. */
int
clink_lock(struct clink *clink) {
  struct cserver *cserver;

  cserver = clink->cserver;
  if (cserver->lock) {
    return CONFIG_FAILURE;
  }

  cserver->lock = 1;
  clink->lock = 1;

  return CONFIG_SUCCESS;
}

/* Unlock the config. */
int
clink_unlock(struct clink *clink) {
  struct cserver *cserver;

  cserver = clink->cserver;
  cserver->lock = 0;
  clink->lock = 0;

  return CONFIG_SUCCESS;
}

/* Read from clink. */
static void
clink_read(struct event *event) {
  ssize_t nbytes;
  struct clink *clink;
  struct pbuf *pbuf;
  struct confsys_header header;
  size_t read_size;
  char *ep;
  struct event_manager *em;
  int sock;


  /* Get parameter. */
  em = event_get_manager(event);
  sock = event_get_sock(event);
  clink = event_get_arg(event);
  pbuf = clink->pbuf;

  /* Clear read event. */
  clink->read_ev = NULL;

  /* Read type and length. */
  pbuf_reset(pbuf);
  nbytes = pbuf_read_size(clink->pbuf, clink->sock, CONFSYS_HEADER_LEN);
  if (nbytes <= 0 || nbytes != CONFSYS_HEADER_LEN) {
    clink_stop(clink);
    return;
  }

  /* Decode type and length. */
  DECODE_GETW(header.type);
  DECODE_GETW(header.length);

  /* Decode value. */
  read_size = (size_t)(header.length - CONFSYS_HEADER_LEN);
  if (read_size > 0) {
    nbytes = pbuf_read_size(clink->pbuf, clink->sock, read_size);
    if (nbytes <= 0 || nbytes != (ssize_t)read_size) {
      clink_stop(clink);
      return;
    }
  }

  /* Register read event. */
  clink->read_ev = event_register_read(em, sock, clink_read, clink);

  /* NULL terminate value. */
  ep = (char *)pbuf->putp;
  *ep = '\0';

  /* Process the message. */
  clink_process(clink, header.type, (char *)pbuf->getp);
}

/* Start clink processing. */
void
clink_start(struct clink *clink, int sock, struct cserver *cserver) {
  struct event_manager *em;

  clink->sock = sock;
  clink->cserver = cserver;

  em = cserver->em;
  clink->read_ev = event_register_read(em, sock, clink_read, clink);

  TAILQ_INSERT_TAIL(&cserver->clink, clink, entry);
}

/* Stop clink, close socket and reset buffer. */
void
clink_stop(struct clink *clink) {
  struct cserver *cserver;

  cserver = clink->cserver;

  pbuf_reset(clink->pbuf);
  pbuf_list_reset(clink->pbuf_list);

  clink_unlock(clink);

  if (clink->sock >= 0) {
    close(clink->sock);
    clink->sock = -1;

    TAILQ_REMOVE(&cserver->clink, clink, entry);
    TAILQ_INSERT_TAIL(&cserver->unused, clink, entry);
  }

  if (clink->read_ev) {
    event_cancel(&clink->read_ev);
  }
  if (clink->write_ev) {
    event_cancel(&clink->write_ev);
  }
}
