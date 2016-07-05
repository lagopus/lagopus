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

/**
 * @file	pbuf.h
 * @brief	Packet buffer.
 */

#ifndef __LAGOPUS_PBUF_H__
#define __LAGOPUS_PBUF_H__

#include "lagopus_apis.h"
#include "lagopus_session.h"

/* pbuf minimum data size. */
#define PBUF_MIN_SIZE   1024

/* pbuf unused buffer try get count. */
#define PBUF_TRY_COUNT     5

struct pbuf {
  /* Linked list entry. */
  TAILQ_ENTRY(pbuf) entry;

  /* reference counter */

  uint32_t refs;

  /* Put position. */
  uint8_t *putp;

  /* Get position. */
  uint8_t *getp;

  /* Packet length. */
  size_t plen;

  /* Buffer data size. */
  size_t size;

  /* Data block. */
  uint8_t data[];
};

/* for save pbuf info. */
typedef struct pbuf pbuf_info_t;

TAILQ_HEAD(pbuf_tailq, pbuf);

/* pbuf_list. */
struct pbuf_list {
  /* Size of the contents. */
  size_t contents_size;

  /* Active buffer. */
  struct pbuf_tailq tailq;

  /* Unused buffer. */
  struct pbuf_tailq unused;
};

#define DECODE_GET(P,L)                                                       \
  do {                                                                        \
    memcpy((void *)(P), pbuf->getp, (L));                                     \
    pbuf->getp += (L);                                                        \
    pbuf->plen -= (L);                                                        \
  } while (0)

#define DECODE_GETC(V)                                                        \
  do {                                                                        \
    (V) = *(pbuf->getp);                                                      \
    pbuf->getp++;                                                             \
    pbuf->plen--;                                                             \
  } while (0)

#define DECODE_GETW(V)                                                        \
  do {                                                                        \
    (V) = (uint16_t)(((*(pbuf->getp)) << 8)                                   \
                     + (*(pbuf->getp + 1)));                                  \
    pbuf->getp += 2;                                                          \
    pbuf->plen -= 2;                                                          \
  } while (0)

#define DECODE_GETL(V)                                                        \
  do {                                                                        \
    (V) = (uint32_t)(((*(pbuf->getp)) << 24)                                  \
                     + ((*(pbuf->getp + 1)) << 16)                            \
                     + ((*(pbuf->getp + 2)) << 8)                             \
                     +  (*(pbuf->getp + 3)));                                 \
    pbuf->getp += 4;                                                          \
    pbuf->plen -= 4;                                                          \
  } while (0)

#define DECODE_GETLL(V)                                                       \
  do {                                                                        \
    (V) = ((uint64_t)(*(pbuf->getp))     << 56)                               \
          + ((uint64_t)(*(pbuf->getp + 1)) << 48)                               \
          + ((uint64_t)(*(pbuf->getp + 2)) << 40)                               \
          + ((uint64_t)(*(pbuf->getp + 3)) << 32)                               \
          + ((uint64_t)(*(pbuf->getp + 4)) << 24)                               \
          + ((uint64_t)(*(pbuf->getp + 5)) << 16)                               \
          + ((uint64_t)(*(pbuf->getp + 6)) << 8)                                \
          +  (uint64_t)(*(pbuf->getp + 7));                                     \
    pbuf->getp += 8;                                                          \
    pbuf->plen -= 8;                                                          \
  } while (0)

#define DECODE_REWIND(L)                                                      \
  do {                                                                        \
    pbuf->getp -= (L);                                                        \
    pbuf->plen += (L);                                                        \
  } while (0)

#define ENCODE_PUT(P,L)                                                       \
  do {                                                                        \
    memcpy((void *)(pbuf->putp), (void *) (P), (L));                          \
    pbuf->putp += (L);                                                        \
    pbuf->plen -= (L);                                                        \
  } while (0)

#define ENCODE_PUTC(V)                                                        \
  do {                                                                        \
    *(pbuf->putp)     = (uint8_t)((V) & 0xFF);                                \
    pbuf->putp++;                                                             \
    pbuf->plen--;                                                             \
  } while (0)

#define ENCODE_PUTW(V)                                                        \
  do {                                                                        \
    *(pbuf->putp)     = (uint8_t)(((V) >> 8) & 0xFF);                         \
    *(pbuf->putp + 1) = (uint8_t)((V) & 0xFF);                                \
    pbuf->putp += 2;                                                          \
    pbuf->plen -= 2;                                                          \
  } while (0)

#define ENCODE_PUTL(V)                                                        \
  do {                                                                        \
    *(pbuf->putp)     = (uint8_t)(((V) >> 24) & 0xFF);                        \
    *(pbuf->putp + 1) = (uint8_t)(((V) >> 16) & 0xFF);                        \
    *(pbuf->putp + 2) = (uint8_t)(((V) >> 8) & 0xFF);                         \
    *(pbuf->putp + 3) = (uint8_t)((V) & 0xFF);                                \
    pbuf->putp += 4;                                                          \
    pbuf->plen -= 4;                                                          \
  } while (0)

#define ENCODE_PUTLL(V)                                                       \
  do {                                                                        \
    *(pbuf->putp)     = (uint8_t)(((V) >> 56) & 0xFF);                        \
    *(pbuf->putp + 1) = (uint8_t)(((V) >> 48) & 0xFF);                        \
    *(pbuf->putp + 2) = (uint8_t)(((V) >> 40) & 0xFF);                        \
    *(pbuf->putp + 3) = (uint8_t)(((V) >> 32) & 0xFF);                        \
    *(pbuf->putp + 4) = (uint8_t)(((V) >> 24) & 0xFF);                        \
    *(pbuf->putp + 5) = (uint8_t)(((V) >> 16) & 0xFF);                        \
    *(pbuf->putp + 6) = (uint8_t)(((V) >> 8) & 0xFF);                         \
    *(pbuf->putp + 7) = (uint8_t)((V) & 0xFF);                                \
    pbuf->putp += 8;                                                          \
    pbuf->plen -= 8;                                                          \
  } while (0)

struct pbuf *
pbuf_alloc(size_t size);

void
pbuf_free(struct pbuf *pbuf);

void
pbuf_reset(struct pbuf *pbuf);

ssize_t
pbuf_read(struct pbuf *pbuf, int sock);

ssize_t
pbuf_read_size(struct pbuf *pbuf, int sock, size_t read_size);

ssize_t
pbuf_session_read(struct pbuf *pbuf, lagopus_session_t session);

size_t
pbuf_readable_size(struct pbuf *pbuf);

size_t
pbuf_writable_size(struct pbuf *pbuf);

void
pbuf_trim_readed(struct pbuf *pbuf);

lagopus_result_t
pbuf_forward(struct pbuf *pbuf, size_t size);

struct pbuf_list *
pbuf_list_alloc(void);

void
pbuf_list_free(struct pbuf_list *pbuf_list);

struct pbuf *
pbuf_list_get(struct pbuf_list *pbuf_list, size_t size);

void
pbuf_list_unget(struct pbuf_list *pbuf_list, struct pbuf *pbuf);

ssize_t
pbuf_list_write(struct pbuf_list *pbuf_list, int sock);

ssize_t
pbuf_list_session_write(struct pbuf_list *pbuf_list,
                        lagopus_session_t session);

struct pbuf *
pbuf_list_first(struct pbuf_list *pbuf_list);

void
pbuf_list_add(struct pbuf_list *pbuf_list, struct pbuf *pbuf);

void
pbuf_list_reset(struct pbuf_list *pbuf_list);

void
pbuf_copy_unread_data(struct pbuf *dst, struct pbuf *src, size_t off);

lagopus_result_t
pbuf_copy(struct pbuf *dst, struct pbuf *src);

lagopus_result_t
pbuf_copy_with_length(struct pbuf *dst, struct pbuf *src,
                      size_t length);

struct pbuf *
pbuf_list_last_get(struct pbuf_list *pbuf_list);

void
pbuf_get(struct pbuf *pbuf);

void
pbuf_put(struct pbuf *pbuf);

void
pbuf_plen_reset(struct pbuf *pbuf);

lagopus_result_t
pbuf_plen_check(struct pbuf *pbuf, size_t len);

lagopus_result_t
pbuf_plen_equal_check(struct pbuf *pbuf, size_t len);

/**
 * Get length (uint16_t).
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[out]	len	A pointer to pbuf length.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Bad length.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
pbuf_length_get(struct pbuf *pbuf, uint16_t *len);

/**
 * Get plen in pbuf.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	length
 */
size_t
pbuf_plen_get(struct pbuf *pbuf);

/**
 * Set plen in pbuf.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	len	length.
 *
 *     @retval	void
 */
void
pbuf_plen_set(struct pbuf *pbuf, size_t len);

/**
 * Get size in pbuf.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	size
 */
size_t
pbuf_size_get(struct pbuf *pbuf);

/**
 * Get getp in pbuf.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	getp
 */
uint8_t *
pbuf_getp_get(struct pbuf *pbuf);

/**
 * Set getp in pbuf.
 *
 *     @param[in,out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	p	A pointer to value.
 *
 *     @retval	void
 */
void
pbuf_getp_set(struct pbuf *pbuf, uint8_t *p);

/**
 * Get putp in pbuf.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	putp
 */
uint8_t *
pbuf_putp_get(struct pbuf *pbuf);

/**
 * Set putp in pbuf.
 *
 *     @param[in,out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	p	A pointer to value.
 *
 *     @retval	void
 */
void
pbuf_putp_set(struct pbuf *pbuf, uint8_t *p);

/**
 * Get data in pbuf.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	data	A pointer to data in pbuf.
 */
uint8_t *
pbuf_data_get(struct pbuf *pbuf);

/**
 * Copy to pbuf from data for encode.
 *
 *     @param[in,out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	data	A pointer to copy data.
 *     @param[in]	length	Size of data.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Bad length.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
pbuf_encode(struct pbuf *pbuf, const void *data,
            size_t length);

/**
 * Store pbuf info(putp, getp, plen).
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in,out]	pbuf_info	A pointer to \e pbuf info structure.
 *
 *     @retval	void
 */
void
pbuf_info_store(struct pbuf *pbuf, pbuf_info_t *pbuf_info);

/**
 * Load pbuf info(putp, getp, plen).
 *
 *     @param[in,out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	pbuf_info	A pointer to \e pbuf info structure.
 *
 *     @retval	void
 */
void
pbuf_info_load(struct pbuf *pbuf, pbuf_info_t *pbuf_info);

#endif /* __LAGOPUS_PBUF_H__ */
