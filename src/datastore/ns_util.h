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
 * @file	ns_util.h
 */

#ifndef __NS_UTIL_H__
#define __NS_UTIL_H__

static inline lagopus_result_t
ns_create_fullname(const char *ns, const char *name, char **fullname) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *buf = NULL;
  size_t full_len = 0;
  size_t ns_len = 0;
  size_t delim_len = 0;
  size_t name_len = 0;

  if (ns != NULL && IS_VALID_STRING(name) == true && fullname != NULL) {
    if (ns[0] == '\0') { // default namespace
      delim_len = strlen(DATASTORE_NAMESPACE_DELIMITER);
      name_len = strlen(name);

      full_len = delim_len + name_len;

      buf = (char *) malloc(sizeof(char) * (full_len + 1));
      if (buf != NULL) {
        ret = snprintf(buf, full_len + 1, "%s%s",
                       DATASTORE_NAMESPACE_DELIMITER, name);
        if (ret >= 0) {
          *fullname = buf;
          ret = LAGOPUS_RESULT_OK;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ns_len = strlen(ns);
      delim_len = strlen(DATASTORE_NAMESPACE_DELIMITER);
      name_len = strlen(name);

      full_len = ns_len + delim_len + name_len;

      buf = (char *) malloc(sizeof(char) * (full_len + 1));
      ret = snprintf(buf, full_len + 1, "%s%s%s",
                     ns, DATASTORE_NAMESPACE_DELIMITER, name);
      if (ret >= 0) {
        *fullname = buf;
        ret = LAGOPUS_RESULT_OK;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
ns_split_fullname(const char *fullname, char **ns, char **name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t fullname_len = 0;
  size_t delim_len = 0;
  size_t ns_len = 0;
  size_t name_len = 0;
  char *ns_buf = NULL;
  char *name_buf = NULL;

  if (IS_VALID_STRING(fullname) == true && ns != NULL && name != NULL) {
    ret = lagopus_str_indexof(fullname, DATASTORE_NAMESPACE_DELIMITER);

    if (ret >= 0) { // namespace exists
      fullname_len = strlen(fullname);
      delim_len = strlen(DATASTORE_NAMESPACE_DELIMITER);
      ns_len = (size_t) ret;
      name_len = fullname_len - delim_len - ns_len;

      ns_buf = (char *) malloc(sizeof(char) * (ns_len + 1));
      if (ns_buf != NULL) {
        ret = snprintf(ns_buf, ns_len + 1, "%s", fullname);
        if (ret >= 0) {
          *ns = ns_buf;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          goto error;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto error;
      }

      name_buf = (char *) malloc(sizeof(char) * (name_len + 1));
      if (name_buf != NULL) {
        ret = snprintf(name_buf, name_len + 1, "%s",
                       fullname + ns_len + delim_len);
        if (ret >= 0) {
          *name = name_buf;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          goto error;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto error;
      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;

error:
  free(ns_buf);
  free(name_buf);
  *ns = NULL;
  *name = NULL;
  return ret;
}

static inline lagopus_result_t
ns_get_namespace(const char *fullname, const char *current_ns, char **ns) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *ns_buf = NULL;

  if (IS_VALID_STRING(fullname) == true && current_ns != NULL && ns != NULL) {
    ret = lagopus_str_indexof(fullname, DATASTORE_NAMESPACE_DELIMITER);

    if (ret >= 0) { // namespace exists
      size_t i = 0;
      size_t idx = 0;
      size_t ns_len = 0;

      ns_buf = strdup(fullname);
      if (IS_VALID_STRING(ns_buf) == true) {
        idx = (size_t) ret;
        ns_len = strlen(ns_buf);

        for (i = idx; i < ns_len; i++) {
          ns_buf[i] = '\0';
        }

        *ns = ns_buf;

        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else { // no namespace
      if (current_ns[0] == '\0') { // default namespace
        ns_buf = (char *) malloc(sizeof(char));
        if (ns_buf != NULL) {
          ns_buf[0] = '\0';
          *ns = ns_buf;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }
      } else {
        ns_buf = strdup(current_ns);
        if (IS_VALID_STRING(ns_buf) == true) {
          *ns = ns_buf;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
ns_replace_namespace(const char *fullname, const char *ns,
                     char **new_fullname) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *ns_buf = NULL;
  char *name_buf = NULL;
  char *fullname_buf = NULL;

  if (IS_VALID_STRING(fullname) == true && ns != NULL && new_fullname != NULL) {
    ret = ns_split_fullname(fullname, &ns_buf, &name_buf);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ns_create_fullname(ns, name_buf, &fullname_buf);
      if (ret == LAGOPUS_RESULT_OK) {
        *new_fullname = fullname_buf;
      } else {
        free(fullname_buf);
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  free(ns_buf);
  free(name_buf);

  return ret;
}

#endif /* __NS_UTIL_H__ */
