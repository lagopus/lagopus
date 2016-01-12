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
#include "lagopus_dstring.h"

#define NULL_STR_SIZE 1   /* size of '\0'. */
#define ALLOC_SIZE(_n) (sizeof(char) * (_n))

#define DS_VSNPRINTF(_ds, _size, _format, _args)                \
  vsnprintf((_ds)->str + (_ds)->str_size,                       \
            (_size), (_format), (_args))

#define DS_STRDUP(_ds, _offset)                                 \
  (((_ds)->str_size == 0) ? strdup("") : strdup((_ds)->str + _offset))

struct dstring {
  size_t size; /* size of the allocation. */
  size_t str_size; /* size of str without '\0'. */
  char *str;
};

static inline void
dstring_init(lagopus_dstring_t ds) {
  ds->str = NULL;
  ds->size = 0;
  ds->str_size = 0;
}

static inline void
dstring_reset(lagopus_dstring_t ds) {
  if (ds->str != NULL) {
    /* set '\0'. */
    ds->str[0] = '\0';
    ds->str_size = 0;
  }
}

static inline void
str_free(lagopus_dstring_t ds) {
  if (ds->str != NULL) {
    free(ds->str);
    ds->str = NULL;
  }
}

static inline lagopus_result_t
str_realloc(lagopus_dstring_t ds, size_t size) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *tmp = NULL;
  size_t alloc_size = ds->str_size + size + NULL_STR_SIZE;

  if (ds->str == NULL ||
      (size != 0 && ds->size < alloc_size)) {
    tmp = (char *) realloc(ds->str, ALLOC_SIZE(alloc_size));
    if (tmp != NULL) {
      /* set dstring fields. */
      ds->str = tmp;
      ds->size = alloc_size;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    /* not need allocation. */
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_create(lagopus_dstring_t *ds) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ds != NULL) {
    *ds = (struct dstring *) malloc(sizeof(struct dstring));

    if (*ds == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    } else {
      dstring_init(*ds);
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

void
lagopus_dstring_destroy(lagopus_dstring_t *ds) {
  if (ds != NULL && *ds != NULL) {
    str_free(*ds);
    free(*ds);
    *ds = NULL;
  }
}

lagopus_result_t
lagopus_dstring_clear(lagopus_dstring_t *ds) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ds != NULL && *ds != NULL) {
    dstring_reset(*ds);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_vappendf(lagopus_dstring_t *ds, const char *format,
                         va_list *args) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  va_list cpy_args;
  int pre_size, size;

  if (ds != NULL && *ds != NULL &&
      format != NULL && args != NULL) {
    va_copy(cpy_args, *args);

    pre_size = DS_VSNPRINTF(*ds, 0, format, *args);

    if (pre_size >= 0) {
      ret = str_realloc(*ds, (size_t) pre_size);
      if (ret == LAGOPUS_RESULT_OK) {
        size = DS_VSNPRINTF(*ds, (size_t) (pre_size + NULL_STR_SIZE),
                            format, cpy_args);
        if (size >= 0 && size == pre_size) {
          /* sum size of str. */
          (*ds)->str_size += (size_t) size;
          ret = LAGOPUS_RESULT_OK;
        } else {
          /* free. */
          str_free(*ds);
          ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }

    va_end(cpy_args);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_appendf(lagopus_dstring_t *ds, const char *format, ...) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  va_list args;

  if (ds != NULL && *ds != NULL && format != NULL) {
    va_start(args, format);
    ret = lagopus_dstring_vappendf(ds, format, &args);
    va_end(args);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_vprependf(lagopus_dstring_t *ds, const char *format,
                          va_list *args) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  va_list cpy_args;
  int pre_size, size;
  char *cpy_str = NULL;

  if (ds != NULL && *ds != NULL &&
      format != NULL && args != NULL) {
    va_copy(cpy_args, *args);

    pre_size = DS_VSNPRINTF(*ds, 0, format, *args);

    if (pre_size >= 0) {
      ret = str_realloc(*ds, (size_t) pre_size);
      if (ret == LAGOPUS_RESULT_OK) {
        cpy_str = DS_STRDUP(*ds, 0);
        if (cpy_str != NULL) {
          size = vsnprintf((*ds)->str, (size_t) (pre_size + NULL_STR_SIZE),
                           format, cpy_args);
          if (size >= 0 && size == pre_size) {
            memmove((*ds)->str + size, cpy_str, (*ds)->str_size);
            /* sum size of str. */
            (*ds)->str_size += (size_t) size;
            *((*ds)->str + (*ds)->str_size) = '\0';
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          }
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }
    va_end(cpy_args);

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      str_free(*ds);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  free(cpy_str);

  return ret;
}

lagopus_result_t
lagopus_dstring_prependf(lagopus_dstring_t *ds, const char *format, ...) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  va_list args;

  if (ds != NULL && *ds != NULL && format != NULL) {
    va_start(args, format);
    ret = lagopus_dstring_vprependf(ds, format, &args);
    va_end(args);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_vinsertf(lagopus_dstring_t *ds,
                         size_t offset,
                         const char *format,
                         va_list *args) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  va_list cpy_args;
  int pre_size, size;
  char *cpy_str = NULL;

  if (ds != NULL && *ds != NULL &&
      format != NULL && args != NULL &&
      (*ds)->str_size >= offset) {
    va_copy(cpy_args, *args);

    pre_size = DS_VSNPRINTF(*ds, 0, format, *args);

    if (pre_size >= 0) {
      ret = str_realloc(*ds, (size_t) pre_size);
      if (ret == LAGOPUS_RESULT_OK) {
        cpy_str = DS_STRDUP(*ds, offset);
        if (cpy_str != NULL) {
          size = vsnprintf((*ds)->str + offset,
                           (size_t) (pre_size + NULL_STR_SIZE),
                           format, cpy_args);
          if (size >= 0 && size == pre_size) {
            memmove((*ds)->str + size + offset,
                    cpy_str,
                    (*ds)->str_size - offset);
            /* sum size of str. */
            (*ds)->str_size += (size_t) size;
            *((*ds)->str + (*ds)->str_size) = '\0';
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          }
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }
    va_end(cpy_args);

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      str_free(*ds);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  free(cpy_str);

  return ret;
}

lagopus_result_t
lagopus_dstring_insertf(lagopus_dstring_t *ds,
                        size_t offset,
                        const char *format, ...) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  va_list args;

  if (ds != NULL && *ds != NULL && format != NULL) {
    va_start(args, format);
    ret = lagopus_dstring_vinsertf(ds, offset, format, &args);
    va_end(args);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_concat(lagopus_dstring_t *dst_ds,
                       const lagopus_dstring_t *src_ds) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (dst_ds != NULL && *dst_ds != NULL &&
      src_ds != NULL && *src_ds != NULL) {
    ret = lagopus_dstring_appendf(dst_ds, "%s", (*src_ds)->str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_str_get(lagopus_dstring_t *ds, char **str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ds != NULL && *ds != NULL && str != NULL) {
    *str = DS_STRDUP(*ds, 0);

    if (*str == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
lagopus_dstring_len_get(lagopus_dstring_t *ds) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ds != NULL && *ds != NULL) {
    ret = (ssize_t) (*ds)->str_size;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

bool
lagopus_dstring_empty(lagopus_dstring_t *ds) {
  bool ret = true;

  if (ds != NULL && *ds != NULL) {
    if ((*ds)->str_size != 0) {
      ret = false;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
