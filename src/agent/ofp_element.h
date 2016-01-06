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
 * @file	ofp_element.h
 */

#ifndef __OFP_ELEMENT_H__
#define __OFP_ELEMENT_H__

/**
 * @brief	bitmap
 * @details	bitmap in element.
 */
struct bitmap {
  TAILQ_ENTRY(bitmap) entry;
  uint32_t bitmap;
};
TAILQ_HEAD(bitmap_list, bitmap);

/**
 * @brief	element
 * @details	element in ofp_hello.
 */
struct element {
  TAILQ_ENTRY(element) entry;
  uint16_t type;
  uint16_t length;

  /* for ofp_hello_elem_versionbitmap. */
  struct bitmap_list bitmap_list;
};
TAILQ_HEAD(element_list, element);

/**
 * Alloc bitmap.
 *
 *     @retval	*bitmap	Succeeded, A pointer to \e bitmap structure.
 *     @retval	NULL	Failed.
 */
struct bitmap *
bitmap_alloc(void);

/**
 * Free bitmap_list elements.
 *
 *     @param[in]	bitmap_list	A pointer to list of
 *     \e bitmap structures.
 *
 *     @retval	void
 */
void
bitmap_list_elem_free(struct bitmap_list *bitmap_list);

/**
 * Alloc element.
 *
 *     @retval	*element	Succeeded, A pointer to \e element structure.
 *     @retval	NULL	Failed.
 */
struct element *
element_alloc(void);

/**
 * Free element_list elements.
 *
 *     @param[in]	element_list	A pointer to list of
 *     \e element structures.
 *
 *     @retval	void
 */
void
element_list_elem_free(struct element_list *element_list);

/**
 * Parse element.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[out]	element_list	A pointer to list of \e element structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_element_parse(struct pbuf *pbuf,
                  struct element_list *element_list,
                  struct ofp_error *error);

/**
 * Encode element_list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	element_list	A pointer to list of \e element structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_element_list_encode(struct pbuf *pbuf,
                        struct element_list *element_list);

#endif /* __OFP_ELEMENT_H__ */
