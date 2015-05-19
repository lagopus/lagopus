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


#ifndef SRC_CONFIG_CXML_H_
#define SRC_CONFIG_CXML_H_

#include "lagopus/queue.h"
#include <expat.h>

TAILQ_HEAD(attr_list, cxml_attribute);
TAILQ_HEAD(element_list, cxml_element);

struct cxml_attribute {
  TAILQ_ENTRY(cxml_attribute) entry;

  const char *name;
  const char *value;
};

struct cxml_element {
  TAILQ_ENTRY(cxml_element) entry;

  struct cxml_element *parent;

  const char *name;
  const char *value;

  struct attr_list attr_list;

  struct element_list ele_list;
};

struct cxml_document {
  /* Root element. */
  struct cxml_element *root;

  /* Current element path. */
  struct element_list cpath;
};

typedef void
(*sax_callback_t)(struct cxml_element *ele, void *user_data);

void
cxml_sax(struct cxml_document *doc, sax_callback_t start_cb,
         sax_callback_t character_cb, sax_callback_t end_cb, void *user_data);


struct cxml_document *
cxml_document_read(const char *file_name);

void
cxml_debug(struct cxml_document *doc);

const char *
cxml_attribute_value(struct cxml_element *ele, const char *name);

#endif /* SRC_CONFIG_CXML_H_ */
