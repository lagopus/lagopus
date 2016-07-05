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
#include "lagopus_session.h"
#include "lagopus/pbuf.h"
#include "openflow.h"

/* Reset pbuf. */
void
pbuf_reset(struct pbuf *pbuf) {
  pbuf->getp = pbuf->data;
  pbuf->putp = pbuf->data;
  pbuf->plen = 0;
}

/* Allocate a pbuf. */
struct pbuf *
pbuf_alloc(size_t size) {
  struct pbuf *pbuf;

  /* Adjust size to minimum buffer size. */
  if (size < PBUF_MIN_SIZE) {
    size = PBUF_MIN_SIZE;
  }

  pbuf = (struct pbuf *)calloc(1, sizeof(struct pbuf) + size);
  if (pbuf == NULL) {
    return NULL;
  }

  pbuf->size = size;
  pbuf->refs = 1;
  pbuf_reset(pbuf);

  return pbuf;
}

/* increment reference counter */
void
pbuf_get(struct pbuf *pbuf) {
  pbuf->refs++;
}

/* decrement reference counter */
void
pbuf_put(struct pbuf *pbuf) {
  assert(pbuf->refs != 0);
  pbuf->refs--;
}

/* Free pbuf. */
void
pbuf_free(struct pbuf *pbuf) {
  if (pbuf == NULL) {
    return;
  }
  assert(pbuf->refs != 0);
  pbuf->refs--;
  if (pbuf->refs == 0) {
    free(pbuf);
  }
}

/* Return readable size of the pbuf. */
size_t
pbuf_readable_size(struct pbuf *pbuf) {
  return (size_t)(pbuf->putp - pbuf->getp);
}

/* Return readable size of the pbuf. */
size_t
pbuf_writable_size(struct pbuf *pbuf) {
  return (size_t)((pbuf->data + pbuf->size) - pbuf->putp);
}

/* Read packet into pbuf. */
ssize_t
pbuf_read(struct pbuf *pbuf, int sock) {
  ssize_t nbytes;
  size_t read_size;

  /* Calculate read size. */
  read_size = pbuf->size - (size_t)(pbuf->putp - pbuf->data);

  nbytes = read(sock, pbuf->putp, read_size);
  if (nbytes > 0) {
    pbuf->putp += nbytes;
  }

  return nbytes;
}

/* Read packet into pbuf. */
ssize_t
pbuf_read_size(struct pbuf *pbuf, int sock, size_t read_size) {
  ssize_t nbytes;

  nbytes = read(sock, pbuf->putp, read_size);
  if (nbytes > 0) {
    pbuf->putp += nbytes;
  }

  return nbytes;
}

/* Session_read packet into pbuf. */
ssize_t
pbuf_session_read(struct pbuf *pbuf, lagopus_session_t session) {
  ssize_t nbytes;
  size_t read_size;

  /* Calculate read size. */
  read_size = pbuf->size - (size_t)(pbuf->putp - pbuf->data);

  nbytes = session_read(session, pbuf->putp, read_size);
  if (nbytes > 0) {
    pbuf->putp += nbytes;
  }

  return nbytes;
}

/* Trim readed data. */
void
pbuf_trim_readed(struct pbuf *pbuf) {
  size_t head_gap = (size_t)(pbuf->getp - pbuf->data);
  size_t tail_gap = (size_t)(pbuf->putp - pbuf->getp);

  /* Move forward unreaded data when it exists. */
  if (tail_gap != 0) {
    memmove(pbuf->data, pbuf->getp, tail_gap);
  }

  /* Update get and put pointer. */
  pbuf->getp -= head_gap;
  pbuf->putp -= head_gap;
}

/* Forward pbuf. */
lagopus_result_t
pbuf_forward(struct pbuf *pbuf, size_t size) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL) {
    ret = pbuf_plen_check(pbuf, size);
    if (ret == LAGOPUS_RESULT_OK) {
      pbuf->getp += size;
      pbuf->plen -= size;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Copy pbuf for internal. */
static size_t
pbuf_copy_internal(struct pbuf *dst, struct pbuf *src,
                   size_t length, size_t off) {
  size_t tail_gap = length - off;

  /* Copy forward unreaded data to other new pbuf when it exists. */
  if (tail_gap != 0) {
    memcpy(dst->putp, src->getp + off, tail_gap);
  }
  dst->putp += tail_gap;
  dst->plen += tail_gap;

  assert(dst->data + dst->size >= dst->putp);

  return tail_gap;
}

/* Copy unread data to other new pbuf */
void
pbuf_copy_unread_data(struct pbuf *dst, struct pbuf *src, size_t off) {
  size_t length = (size_t) (src->putp - src->getp);
  size_t tail_gap;

  tail_gap = pbuf_copy_internal(dst, src,
                                length, off);
  /* backward src putp. */
  src->putp -= tail_gap;
}

/* Copy pbuf */
lagopus_result_t
pbuf_copy(struct pbuf *dst, struct pbuf *src) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t length;

  if (dst != NULL && src != NULL) {
    length = (size_t) (src->putp - src->getp);

    if ((src->getp <= src->putp) &&
        (src->getp + length == src->putp) &&
        length <= dst->size) {
      (void) pbuf_copy_internal(dst, src,
                                length, 0);
      ret = LAGOPUS_RESULT_OK;
    } else {
      lagopus_msg_warning("over length.\n");
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Copy pbuf with length. */
lagopus_result_t
pbuf_copy_with_length(struct pbuf *dst, struct pbuf *src,
                      size_t length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t pbuf_length;

  if (dst != NULL && src != NULL) {
    pbuf_length = (size_t) (src->putp - src->getp);

    /* check over length. */
    if (pbuf_length < length) {
      /* cut length. */
      length = pbuf_length;
    }

    if ((src->getp <= src->putp) &&
        (src->getp + pbuf_length == src->putp) &&
        length <= dst->size) {
      (void) pbuf_copy_internal(dst, src,
                                length, 0);
      ret = LAGOPUS_RESULT_OK;
    } else {
      lagopus_msg_warning("over length.\n");
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Encode pbuf. */
lagopus_result_t
pbuf_encode(struct pbuf *pbuf, const void *data,
            size_t length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL && data != NULL) {
    /* check over length. */
    ret = pbuf_plen_check(pbuf, length);

    if (ret == LAGOPUS_RESULT_OK) {
      memcpy(pbuf->putp, data, length);
      pbuf->putp += length;
      pbuf->plen -= length;
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Reset plen. */
void
pbuf_plen_reset(struct pbuf *pbuf) {
  if (pbuf != NULL) {
    pbuf->plen = 0;
  }
}

/* Check plen. */
lagopus_result_t
pbuf_plen_check(struct pbuf *pbuf, size_t len) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL) {
    if (pbuf->plen < len) {
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Check equal plen. */
lagopus_result_t
pbuf_plen_equal_check(struct pbuf *pbuf, size_t len) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL) {
    if (pbuf->plen != len) {
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Get getp. */
uint8_t *
pbuf_getp_get(struct pbuf *pbuf) {
  return pbuf->getp;
}

/* Set getp. */
void
pbuf_getp_set(struct pbuf *pbuf, uint8_t *p) {
  pbuf->getp = p;
}

/* Get getp. */
uint8_t *
pbuf_putp_get(struct pbuf *pbuf) {
  return pbuf->putp;
}

/* Set puttp. */
void
pbuf_putp_set(struct pbuf *pbuf, uint8_t *p) {
  pbuf->putp = p;
}

/* Get plen. */
size_t
pbuf_plen_get(struct pbuf *pbuf) {
  size_t len = 0;

  if (pbuf != NULL) {
    len = pbuf->plen;
  }

  return len;
}

/* Set plen. */
void
pbuf_plen_set(struct pbuf *pbuf, size_t len) {
  if (pbuf != NULL) {
    pbuf->plen = len;
  }
}

/* Get size. */
size_t
pbuf_size_get(struct pbuf *pbuf) {
  size_t size = 0;

  if (pbuf != NULL) {
    size = pbuf->size;
  }

  return size;
}

/* Get size. */
uint8_t *
pbuf_data_get(struct pbuf *pbuf) {
  return pbuf->data;
}

/* Get length (uint16_t). */
lagopus_result_t
pbuf_length_get(struct pbuf *pbuf, uint16_t *len) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length;

  if (pbuf != NULL && len != NULL) {
    /* check pointer. */
    if (pbuf->getp <= pbuf->putp) {
      length = (uint16_t) (pbuf->putp - pbuf->getp);

      /* check overflow. */
      if (pbuf->getp + length == pbuf->putp) {
        *len = length;
        ret = LAGOPUS_RESULT_OK;
      } else {
        lagopus_msg_warning("overflow pbuf.\n");
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
      }
    } else {
      lagopus_msg_warning("bad getp/putp.\n");
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Store pbuf info. */
void
pbuf_info_store(struct pbuf *pbuf, pbuf_info_t *pbuf_info) {
  if (pbuf != NULL && pbuf_info!= NULL) {
    pbuf_info->putp = pbuf->putp;
    pbuf_info->getp = pbuf->getp;
    pbuf_info->plen = pbuf->plen;
  }
}

/* Load pbuf info. */
void
pbuf_info_load(struct pbuf *pbuf, pbuf_info_t *pbuf_info) {
  if (pbuf != NULL && pbuf_info!= NULL) {
    pbuf->putp = pbuf_info->putp;
    pbuf->getp = pbuf_info->getp;
    pbuf->plen = pbuf_info->plen;
  }
}


/* Allocate pbuf list. */
struct pbuf_list *
pbuf_list_alloc(void) {
  struct pbuf_list *pbuf_list;

  pbuf_list = (struct pbuf_list *)calloc(1, sizeof(struct pbuf_list));
  if (pbuf_list == NULL) {
    return NULL;
  }

  TAILQ_INIT(&pbuf_list->tailq);
  TAILQ_INIT(&pbuf_list->unused);

  pbuf_list->contents_size = 0;

  return pbuf_list;
}

/* Free pbuf list. */
void
pbuf_list_free(struct pbuf_list *pbuf_list) {
  struct pbuf *pbuf;
  if (pbuf_list != NULL) {
    while ((pbuf = TAILQ_FIRST(&pbuf_list->tailq)) != NULL) {
      TAILQ_REMOVE(&pbuf_list->tailq, pbuf, entry);
      pbuf_free(pbuf);
    }
    while ((pbuf = TAILQ_FIRST(&pbuf_list->unused)) != NULL) {
      TAILQ_REMOVE(&pbuf_list->unused, pbuf, entry);
      pbuf_free(pbuf);
    }
    free(pbuf_list);
  }
}

/* Get pbuf from pbuf_list. */
struct pbuf *
pbuf_list_get(struct pbuf_list *pbuf_list, size_t size) {
  struct pbuf *pbuf;
  int try_count;

  /* Try to get size buffer from unused list. */
  pbuf = NULL;
  try_count = 0;
  TAILQ_FOREACH(pbuf, &pbuf_list->unused, entry) {
    if (try_count > PBUF_TRY_COUNT) {
      pbuf = NULL;
      break;
    }
    if (pbuf->size >= size) {
      break;
    }
    /* Try to get unused buffer up until try_count. */
    try_count++;
  }

  if (pbuf != NULL) {
    TAILQ_REMOVE(&pbuf_list->unused, pbuf, entry);
    pbuf_reset(pbuf);
  } else {
    pbuf = pbuf_alloc(size);
    if (pbuf == NULL) {
      return NULL;
    }
  }
  return pbuf;
}

/* Unget the pbuf. Return it to pbuf unused list. */
void
pbuf_list_unget(struct pbuf_list *pbuf_list, struct pbuf *pbuf) {
  TAILQ_INSERT_HEAD(&pbuf_list->unused, pbuf, entry);
}

ssize_t
pbuf_list_write(struct pbuf_list *pbuf_list, int sock) {
  ssize_t nbytes;
  size_t write_size;
  struct pbuf *pbuf = TAILQ_FIRST(&pbuf_list->tailq);

  if (pbuf == NULL) {
    return 0;
  }

  write_size = pbuf_readable_size(pbuf);

  nbytes = write(sock, pbuf->getp, write_size);
  if (nbytes > 0) {
    pbuf->getp += nbytes;
  }

  if (pbuf->getp == pbuf->putp) {
    TAILQ_REMOVE(&pbuf_list->tailq, pbuf, entry);
    TAILQ_INSERT_HEAD(&pbuf_list->unused, pbuf, entry);
  }

  return nbytes;
}

ssize_t
pbuf_list_session_write(struct pbuf_list *pbuf_list,
                        lagopus_session_t session) {
  ssize_t nbytes;
  size_t write_size;
  struct pbuf *pbuf = TAILQ_FIRST(&pbuf_list->tailq);

  if (pbuf == NULL) {
    return 0;
  }

  write_size = (size_t)(pbuf->putp - pbuf->getp);

  nbytes = session_write(session, pbuf->getp, write_size);
  if (nbytes > 0) {
    pbuf->getp += nbytes;
  }

  if (pbuf->getp == pbuf->putp) {
    TAILQ_REMOVE(&pbuf_list->tailq, pbuf, entry);
    TAILQ_INSERT_HEAD(&pbuf_list->unused, pbuf, entry);
  }

  return nbytes;
}

/* Return first pbuf in pbuf_list. */
struct pbuf *
pbuf_list_first(struct pbuf_list *pbuf_list) {
  return TAILQ_FIRST(&pbuf_list->tailq);
}

/* Add the buffer to pbuf list. */
void
pbuf_list_add(struct pbuf_list *pbuf_list, struct pbuf *pbuf) {
  TAILQ_INSERT_TAIL(&pbuf_list->tailq, pbuf, entry);
}

/* Get last element in pbuf_list. */
struct pbuf *
pbuf_list_last_get(struct pbuf_list *pbuf_list) {
  struct pbuf *pbuf;

  pbuf = TAILQ_LAST(&pbuf_list->tailq, pbuf_tailq);

  if (pbuf == NULL) {
    pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);
    if (pbuf == NULL) {
      return NULL;
    }
    pbuf_list_add(pbuf_list, pbuf);
  }

  return pbuf;
}

/* Move all of list to unsed. */
void
pbuf_list_reset(struct pbuf_list *pbuf_list) {
  struct pbuf *pbuf;

  while ((pbuf = TAILQ_FIRST(&pbuf_list->tailq)) != NULL) {
    TAILQ_REMOVE(&pbuf_list->tailq, pbuf, entry);
    TAILQ_INSERT_TAIL(&pbuf_list->unused, pbuf, entry);
  }

  pbuf_list->contents_size = 0;
}
