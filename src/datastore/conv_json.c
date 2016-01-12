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

#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "lagopus_dstring.h"
#include "conv_json.h"

#define ESCAPE_STR_LEN 2

static const struct escape_char {
  const char c;
  const char *escape_str;
} escape_chars[] = {
  {'\"', "\\\""},
  {'\\', "\\\\"},
  {'/',  "\\/"},
  {'\b', "\\b"},
  {'\f', "\\f"},
  {'\n', "\\n"},
  {'\r', "\\r"},
  {'\t', "\\t"},
};

static size_t escape_chars_len =
  sizeof(escape_chars) / sizeof(struct escape_char);

static lagopus_result_t
json_string_setf(lagopus_dstring_t *ds, const char *format,
                 va_list *args) __attr_format_printf__(2, 0);

static bool
escape_include(const char c, size_t *index) {
  bool ret = false;
  size_t i;

  for (i = 0; i < escape_chars_len; i++) {
    if (c == escape_chars[i].c) {
      if (index != NULL) {
        *index = i;
      }
      ret = true;
    }
  }

  return ret;
}

lagopus_result_t
datastore_json_string_escape(const char *in_str, char **out_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  size_t len = 0;
  size_t escapes_len = 0;
  char *in_str_cpy = NULL;
  char *cp = NULL;
  char *str = NULL;

  if (in_str != NULL && out_str != NULL && *out_str == NULL) {
    in_str_cpy = strdup(in_str);
    if (in_str_cpy != NULL) {
      cp = in_str_cpy;
      while (*cp != '\0') {
        if (escape_include(*cp, NULL) == true) {
          escapes_len += ESCAPE_STR_LEN;
        } else {
          len++;
        }
        cp++;
      }

      if (escapes_len == 0) {
        /* not include escape characters. */
        *out_str = strdup(in_str_cpy);
        if (*out_str != NULL) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
          lagopus_perror(ret);
        }
      } else {
        *out_str = (char *) malloc(sizeof(char) *
                                   (len + escapes_len + 1));
        if (*out_str != NULL) {
          str = *out_str;
          cp = in_str_cpy;
          while (*cp != '\0') {
            if (escape_include(*cp, &i) == true) {
              memcpy(str, escape_chars[i].escape_str,
                     sizeof(char) * ESCAPE_STR_LEN);
              str += ESCAPE_STR_LEN;
            } else {
              *str = *cp;
              str++;
            }
            cp++;
          }
          (*out_str)[len + escapes_len] = '\0';

          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
          lagopus_perror(ret);
        }
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      free(*out_str);
      *out_str = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  /* free. */
  free(in_str_cpy);

  return ret;
}

lagopus_result_t
datastore_json_result_set_with_log(lagopus_dstring_t *ds,
                                   lagopus_result_t result_code,
                                   const char *file,
                                   int line,
                                   const char *func,
                                   const char *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ds != NULL && *ds != NULL &&
      file != NULL && func != NULL) {
    /* put log. */
    if (result_code < LAGOPUS_RESULT_OK) {
      lagopus_log_emit(LAGOPUS_LOG_LEVEL_ERROR, 0LL, file, line, func,
                       "set datastore json err: %s\n",
                       datastore_json_result_string_get(result_code));
    }

    ret = lagopus_dstring_clear(ds);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = lagopus_dstring_appendf(ds, "{");

      if (ret == LAGOPUS_RESULT_OK) {
        ret = datastore_json_string_append(
                ds, "ret",
                datastore_json_result_string_get(result_code),
                false);
        if (ret == LAGOPUS_RESULT_OK) {
          if (val != NULL) {
            ret = lagopus_dstring_appendf(ds, ",\n\"data\":%s", val);
          }
          if (ret == LAGOPUS_RESULT_OK) {
            ret = lagopus_dstring_appendf(ds, "}");

            if (ret == LAGOPUS_RESULT_OK) {
              if (result_code != LAGOPUS_RESULT_OK) {
                ret = LAGOPUS_RESULT_DATASTORE_INTERP_ERROR;
              }
            } else {
              lagopus_perror(ret);
            }
          } else {
            lagopus_perror(ret);
          }
        } else {
          lagopus_perror(ret);
        }
      } else {
        lagopus_perror(ret);
      }
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

static lagopus_result_t
json_string_setf(lagopus_dstring_t *ds, const char *format,
                 va_list *args) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t escape_ds = NULL;
  char *str = NULL;
  char *escape_str = NULL;

  ret = lagopus_dstring_create(&escape_ds);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_vappendf(&escape_ds, format, args);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = lagopus_dstring_str_get(&escape_ds, &str);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = datastore_json_string_escape(str, &escape_str);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_appendf(ds, "\"%s\"", escape_str);
        }
      }
    }
  }

  lagopus_dstring_destroy(&escape_ds);
  free(str);
  free(escape_str);

  return ret;
}

lagopus_result_t
datastore_json_result_setf_with_log(lagopus_dstring_t *ds,
                                    lagopus_result_t result_code,
                                    bool is_json_string,
                                    const char *file,
                                    int line,
                                    const char *func,
                                    const char *format, ...) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  va_list args;

  if (ds != NULL && *ds != NULL && file != NULL &&
      func != NULL &&format != NULL) {
    va_start(args, format);

    /* put log. */
    if (result_code < LAGOPUS_RESULT_OK) {
      lagopus_log_emit(
          LAGOPUS_LOG_LEVEL_ERROR, 0LL,
          file, line, func,
          "set datastore json err: %s\n",
          datastore_json_result_string_get(result_code));
    }

    ret = lagopus_dstring_clear(ds);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = lagopus_dstring_appendf(ds, "{");

      if (ret == LAGOPUS_RESULT_OK) {
        ret = datastore_json_string_append(
                ds, "ret",
                datastore_json_result_string_get(result_code),
                false);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_appendf(ds, ",\n\"data\":");
          if (ret == LAGOPUS_RESULT_OK) {
            if (is_json_string == true) {
              ret = json_string_setf(ds, format, &args);
            } else {
              ret = lagopus_dstring_vappendf(ds, format, &args);
            }
            if (ret == LAGOPUS_RESULT_OK) {
              ret = lagopus_dstring_appendf(ds, "}");

              if (ret == LAGOPUS_RESULT_OK) {
                if (result_code != LAGOPUS_RESULT_OK) {
                  ret = LAGOPUS_RESULT_DATASTORE_INTERP_ERROR;
                }
              } else {
                lagopus_perror(ret);
              }
            } else {
              lagopus_perror(ret);
            }
          } else {
            lagopus_perror(ret);
          }
        } else {
          lagopus_perror(ret);
        }
      } else {
        lagopus_perror(ret);
      }
    } else {
      lagopus_perror(ret);
    }
    va_end(args);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

lagopus_result_t
datastore_json_string_append(lagopus_dstring_t *ds,
                             const char *key, const char *val, bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;

  if (key != NULL && val != NULL) {
    ret = datastore_json_string_escape(val, &str);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"\"%s\"", key, str);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  free(str);

  return ret;
}

lagopus_result_t
datastore_json_uint64_append(lagopus_dstring_t *ds,
                             const char *key, const uint64_t val,
                             bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"%"PRIu64, key, val);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
datastore_json_uint32_append(lagopus_dstring_t *ds,
                             const char *key, const uint32_t val,
                             bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"%"PRIu32, key, val);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
datastore_json_uint16_append(lagopus_dstring_t *ds,
                             const char *key, const uint16_t val,
                             bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"%"PRIu16, key, val);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
datastore_json_uint8_append(lagopus_dstring_t *ds,
                            const char *key, const uint8_t val,
                            bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"%"PRIu8, key, val);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

#define BOOL_STR(_b) (((_b) == true) ? "true" : "false")

lagopus_result_t
datastore_json_bool_append(lagopus_dstring_t *ds,
                           const char *key, const bool val, bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"%s", key, BOOL_STR(val));
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
datastore_json_string_array_append(lagopus_dstring_t *ds,
                                   const char *key, const char *val[],
                                   const size_t size, bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  if (key != NULL && val != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      if (*val != NULL) {
        for (i = 0; i < size ; i++) {
          ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(i),
                                        val[i]);
          if (ret != LAGOPUS_RESULT_OK) {
            goto done;
          }
        }
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

lagopus_result_t
datastore_json_uint32_flags_append(lagopus_dstring_t *ds,
                                   const char *key,
                                   uint32_t flags,
                                   const char *const strs[],
                                   size_t strs_size,
                                   bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool d;
  size_t i;

  if (ds != NULL && key != NULL && strs != NULL) {
    if ((ret = DSTRING_CHECK_APPENDF(
            ds, delimiter,
            KEY_FMT"[", key)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    d = false;
    for (i = 0; i < strs_size; i++) {
      if ((flags & (uint32_t) (1 << i)) != 0) {
        if ((ret = DSTRING_CHECK_APPENDF(
                ds, d, "\"%s\"", strs[i])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        if (d == false) {
          d = true;
        }
      }
    }

    if ((ret = lagopus_dstring_appendf(ds, "]")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

lagopus_result_t
datastore_json_uint16_flags_append(lagopus_dstring_t *ds,
                                   const char *key,
                                   uint16_t flags,
                                   const char *const strs[],
                                   size_t strs_size,
                                   bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool d;
  size_t i;

  if (ds != NULL && key != NULL && strs != NULL) {
    if ((ret = DSTRING_CHECK_APPENDF(
            ds, delimiter,
            KEY_FMT"[", key)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    d = false;
    for (i = 0; i < strs_size; i++) {
      if ((flags & (uint16_t) (1 << i)) != 0) {
        if ((ret = DSTRING_CHECK_APPENDF(
                ds, d, "\"%s\"", strs[i])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        if (d == false) {
          d = true;
        }
      }
    }

    if ((ret = lagopus_dstring_appendf(ds, "]")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}
