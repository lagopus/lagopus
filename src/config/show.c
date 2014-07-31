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


#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "confsys.h"

/* Write show command output to the socket. */
static void
show_write(struct event *event) {
  struct clink *clink;
  struct event_manager *em;
  ssize_t nbytes;

  /* Get arguments. */
  clink = event_get_arg(event);
  em = event_get_manager(event);

  /* Clear write event since it is now executing ;-). */
  clink->write_ev = NULL;

  /* Write to the socket. */
  nbytes = pbuf_list_write(clink->pbuf_list, clink->sock);
  if (nbytes <= 0) {
    clink_stop(clink);
    return;
  }

  /* Decrement contents size. */
  clink->pbuf_list->contents_size -= nbytes;

  /* Buffer still remains. */
  if (pbuf_list_first(clink->pbuf_list) != NULL) {
    clink->write_ev = event_register_write(em, clink->sock, show_write, clink);
    return;
  }

  /* All buffer is written, close clink now. */
  if (clink->is_show) {
    clink_stop(clink);
  }
}

/* Print string to show connection. */
int
show(struct confsys *confsys, const char *format, ...) {
  va_list args;
  size_t available;
  int to_be_written;
  struct pbuf *pbuf;
  struct clink *clink;
  struct event_manager *em;

  /* Get arguments. */
  clink = confsys->clink;

  /* clink check. */
  if (clink->sock < 0) {
    return 0;
  }

  /* Get event manager. */
  em = clink->cserver->em;

  /* Get last page to write. */
  pbuf = pbuf_list_last(clink->pbuf_list);
  if (pbuf == NULL) {
    return -1;
  }

  /* Remaining writabe size. */
  available = pbuf_writable_size(pbuf);

  /* First attempt to write. */
  va_start(args, format);
  to_be_written = vsnprintf((char *)pbuf->putp, available, format, args);
  va_end(args);

  /* Couldn't write the string. */
  if (to_be_written <= 0) {
    return -1;
  }

  /* Write event on. */
  if (clink->write_ev == NULL) {
    clink->write_ev = event_register_write(em, clink->sock, show_write, clink);
  }

  /* All data is written, return here. */
  if ((size_t)to_be_written < available) {
    /* to be written does not include '\0'.  So this is fine. */
    pbuf->putp += to_be_written;
    clink->pbuf_list->contents_size += to_be_written;
    return to_be_written;
  }

  /* Current buffer is filled.  Reduce avaiable size one for '\0' and
   * add one to to_be_written.  Here, availabe size is same as written
   * size. */
  available--;
  to_be_written++;

  /* Forward current buffer point. */
  pbuf->putp += available;

  /* Prepare additional buffer. */
  pbuf = pbuf_alloc((size_t)to_be_written);
  if (pbuf == NULL) {
    return -1;
  }
  pbuf_list_add(clink->pbuf_list, pbuf);

  /* Print all of data to the buffer. */
  va_start(args, format);
  vsnprintf((char *)pbuf->putp, (size_t)to_be_written, format, args);
  va_end(args);

  pbuf->getp += available;

  /* Adjust to_be_written size for removing '\0'. */
  to_be_written--;
  pbuf->putp += to_be_written;
  clink->pbuf_list->contents_size += to_be_written;

  return to_be_written;
}
