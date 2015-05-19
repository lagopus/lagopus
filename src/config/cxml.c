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
#include <ctype.h>
#include "confsys.h"

/* XML declaration handler. */
static void
declHandler(void *data, const XML_Char *version,
            const XML_Char *encoding, int standalone) {
  struct cxml_document *doc = (struct cxml_document *)data;

  /* Ignore declaration. */
  if (0) {
    printf("Decl cxml_document %p version: %s encoding: %s standalone: %d\n",
           doc, version, encoding, standalone);
  }
}

static struct cxml_element *
cxml_element_alloc(struct cxml_element *parent, const char *name,
                   const char *value) {
  struct cxml_element *ele;

  ele = (struct cxml_element *)calloc(1, sizeof(struct cxml_element));
  if (ele == NULL) {
    return NULL;
  }

  TAILQ_INIT(&ele->attr_list);
  TAILQ_INIT(&ele->ele_list);

  if (name) {
    ele->name = strdup(name);
  }
  if (value) {
    ele->value = strdup(value);
  }

  ele->parent = parent;
  if (parent) {
    TAILQ_INSERT_TAIL(&parent->ele_list, ele, entry);
  }

  return ele;
}

static void
cxml_element_attr_add(struct cxml_element *ele, struct cxml_attribute *attr) {
  TAILQ_INSERT_TAIL(&ele->attr_list, attr, entry);
}

static void
cxml_attribute_add(struct cxml_element *ele, const char *name,
                   const char *value) {
  struct cxml_attribute *attr;

  /* Allocate an attribute. */
  attr = (struct cxml_attribute *)calloc(1, sizeof(struct cxml_attribute));

  /* Store name and value. */
  if (name) {
    attr->name = strdup(name);
  }
  if (value) {
    attr->value = strdup(value);
  }

  /* Add to element. */
  cxml_element_attr_add(ele, attr);
}

const char *
cxml_attribute_value(struct cxml_element *ele, const char *name) {
  struct cxml_attribute *attr;

  TAILQ_FOREACH(attr, &ele->attr_list, entry) {
    if (strncmp(attr->name, name, strlen(name)) == 0) {
      return attr->value;
    }
  }
  return NULL;
}

/* XML tag start handler. */
static void
startHandler(void *data, const XML_Char *name, const XML_Char **attr) {
  struct cxml_document *doc = (struct cxml_document *)data;
  struct cxml_element *ele;

  if (doc->root == NULL) {
    ele = cxml_element_alloc(NULL, name, NULL);
    doc->root = ele;
  } else {
    struct cxml_element *parent;
    parent = TAILQ_LAST(&doc->cpath, element_list);
    ele = cxml_element_alloc(parent, name, NULL);
  }

  TAILQ_INSERT_TAIL(&doc->cpath, ele, entry);

  /* Attributes. */
  for (int i = 0; attr[i]; i += 2) {
    cxml_attribute_add(ele, attr[i], attr[i + 1]);
  }
}

/* Normalize the string. */
static void
normalizeChar(XML_Char *str) {
  size_t start;
  size_t end;
  size_t total;

  /* End is set to total string length. */
  end = strlen(str);

  /* Trim space at the end of the string. */
  while (end > 0 && isspace(str[end - 1])) {
    str[--end] = '\0';
  }

  /* Trim space at the beginning of the string. */
  start = strspn(str, (" \n\r\t\v"));

  /* Calculate new string length. */
  total = end - start;

  /* Clean up. */
  memmove(str, &str[start], total);
  str[total] = '\0';
}

/* XML tag end handler. */
static void
endHandler(void *data, const XML_Char *el) {
  struct cxml_document *doc = (struct cxml_document *)data;
  struct cxml_element *last = TAILQ_LAST(&doc->cpath, element_list);

  /* Fetch value of the element. */
  XML_Char *value = (XML_Char *)last->value;

  /* Argument used. */
  if (0) {
    printf("Ignoring %p\n", el);
  }

  /* When value is there, we need to trim head and tail white space. */
  if (value) {
    normalizeChar(value);

    /* If the length becomes zero, free the value. */
    if (strlen(value) == 0) {
      free(value);
      last->value = NULL;
    }
  }

  /* Pop element stack if it is not empty. */
  if (last != NULL) {
    TAILQ_REMOVE(&doc->cpath, last, entry);
  }
}

/* XML value handler. */
static void
charHandler(void *data, const XML_Char *s, int len) {
  struct cxml_document *doc = (struct cxml_document *)data;
  struct cxml_element *last = TAILQ_LAST(&doc->cpath, element_list);
  char *value;
  size_t current_length = 0;
  size_t new_length;

  /* Return if there is no element in stack. */
  if (last == NULL) {
    return;
  }

  /* Last element's value. */
  value = (char *)last->value;

  /* Current string length. */
  if (value != NULL) {
    current_length = strlen(value);
  }

  /* New length including terminate character. */
  new_length = current_length + (size_t)len + 1;

  /* Memory allocation. */
  if (value == NULL) {
    value = (XML_Char *)calloc(1, new_length);
  } else {
    value = (XML_Char *)realloc(value, new_length);
  }

  /* Set value. */
  last->value = value;

  /* Copy string. */
  if (value) {
    memcpy(value + current_length, s, (size_t)len);
    value[new_length - 1] = '\0';
  }
}

static struct cxml_document *
cxml_document_alloc(void) {
  struct cxml_document *doc;

  doc = calloc(1, sizeof(struct cxml_document));
  if (doc == NULL) {
    return NULL;
  }
  TAILQ_INIT(&doc->cpath);

  return doc;
}

static void
cxml_document_free(struct cxml_document *doc) {
  free(doc);
}

/* Allocate expat XML parser. */
static XML_Parser
cxml_parser_alloc(void *data) {
  XML_Parser xml_parser;

  /* Create expat XML parser. */
  xml_parser = XML_ParserCreate(NULL);
  if (xml_parser == NULL) {
    return NULL;
  }

  /* Set handlers. */
  XML_SetXmlDeclHandler(xml_parser, declHandler);
  XML_SetElementHandler(xml_parser, startHandler, endHandler);
  XML_SetCharacterDataHandler(xml_parser, charHandler);
  XML_SetUserData(xml_parser, data);

  return xml_parser;
}

struct cxml_document *
cxml_document_read(const char *file_name) {
  int ret;
  size_t len;
  char buf[BUFSIZ];
  char default_file[BUFSIZ];
  struct cxml_document *doc = NULL;
  XML_Parser xml_parser = NULL;
  FILE *fp = NULL;

  /* Set user data. */
  doc = cxml_document_alloc();
  if (doc == NULL) {
    fprintf(stderr, "Schema file %s can't read\n", file_name);
    goto error;
  }

  /* Allocate a parser with the document. */
  xml_parser = cxml_parser_alloc((void *)doc);

  /* Open file. */
  fp = fopen(file_name, "r");
  if (fp == NULL) {
    /* Try default path. */
    snprintf(default_file, BUFSIZ, "%s/%s", CONFSYS_LOAD_PATH, file_name);
    fp = fopen(default_file, "r");
    if (fp == NULL) {
      goto error;
    }
  }
  while (!feof(fp)) {
    len = fread(buf, 1, BUFSIZ, fp);

    ret = XML_Parse(xml_parser, buf, (int)len, feof(fp));
    if (ret == XML_STATUS_ERROR) {
      goto error;
    }
  }
  fclose(fp);

  return doc;

error:
  if (doc != NULL) {
    cxml_document_free(doc);
  }
  if (xml_parser != NULL) {
    XML_ParserFree(xml_parser);
  }
  if (fp != NULL) {
    fclose(fp);
  }
  return NULL;
}

static void
cxml_dump_ele(struct cxml_element *ele, int depth) {
  struct cxml_attribute *attr;

  if (depth == 0) {
    printf("<%s", ele->name);
  } else {
    printf("%*s<%s", depth * 2, " ", ele->name);
  }

  TAILQ_FOREACH(attr, &ele->attr_list, entry) {
    printf(" %s=\"%s\"", attr->name, attr->value);
  }

  if (ele->value != NULL) {
    printf(">");
    /*    print_escape(show, value, strlen(value)); */
    printf("</%s>\n", ele->name);
  } else {
    if (TAILQ_EMPTY(&ele->ele_list)) {
      printf("/>\n");
    } else {
      struct cxml_element *child;

      printf(">\n");

      TAILQ_FOREACH(child, &ele->ele_list, entry) {
        cxml_dump_ele(child, depth + 1);
      }

      if (depth == 0) {
        printf("</%s>\n", ele->name);
      } else {
        printf("%*s</%s>\n", depth * 2, " ", ele->name);
      }
    }
  }
}

void
cxml_debug(struct cxml_document *doc) {
  cxml_dump_ele(doc->root, 0);
}

static void
cxml_sax_func(struct cxml_element *ele, sax_callback_t start_cb,
              sax_callback_t character_cb, sax_callback_t end_cb,
              void *user_data, int depth) {
  struct cxml_element *child;

  (*start_cb)(ele, user_data);

  if (ele->value != NULL) {
    if (character_cb != NULL) {
      (*character_cb)(ele, user_data);
    }
  } else {
    TAILQ_FOREACH(child, &ele->ele_list, entry) {
      cxml_sax_func(child, start_cb, character_cb, end_cb, user_data,
                    depth + 1);
    }
  }

  (*end_cb)(ele, user_data);
}

void
cxml_sax(struct cxml_document *doc, sax_callback_t start_cb,
         sax_callback_t character_cb, sax_callback_t end_cb,
         void *user_data) {
  if (doc->root != NULL) {
    cxml_sax_func(doc->root, start_cb, character_cb, end_cb, user_data, 0);
  }
}
