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


#include "lagopus_apis.h"
#include "lagopus/vector.h"

/* Allocate a vector. */
struct vector *
vector_alloc(void) {
  struct vector *v;
  vindex_t size = VECTOR_MIN_SIZE;

  v = calloc(1, sizeof (struct vector));
  if (v == NULL) {
    return NULL;
  }

  v->allocated = size;
  v->max = 0;

  v->index = calloc(1, sizeof(void *) * size);
  if (v->index == NULL) {
    free(v);
    return NULL;
  }

  return v;
}

/* Free vector. */
void
vector_free(struct vector *v) {
  if (v->index != NULL) {
    free(v->index);
  }

  free(v);
}

/* Free vector wrapper only. */
void
vector_wrapper_only_free(struct vector *v) {
  free(v);
}

/* Return vector max. */
uint32_t
vector_max(struct vector *v) {
  return v->max;
}

void *
vector_slot(struct vector *v, uint32_t i) {
  return v->index[i];
}

/* Reset vector. */
void
vector_reset(struct vector *v) {
  v->max = 0;
}

/* Make it sure that vector has at least number slot allocated. */
static bool
vector_ensure(struct vector *v, uint32_t num) {
  if (v->allocated > num) {
    return true;
  }

  /* printf("vector_ensure alloced %u num %u\n", v->allocated, num); */

  v->index = realloc(v->index, sizeof(void *) * (v->allocated * 2));
  if (v->index == NULL) {
    return false;
  }

  memset(&v->index[v->allocated], 0, sizeof(void *) * v->allocated);
  v->allocated *= 2;

  /* printf("vector_ensure alloced %u num %u\n", v->allocated, num); */

  if (v->allocated <= num) {
    return vector_ensure (v, num);
  }

  return true;
}

/* Set value to the vector at the smallest empty slot. */
int
vector_set(struct vector *v, void *val) {
  bool ret;

  /* Ensure next max. */
  ret = vector_ensure(v, v->max + 1);
  if (ret != true) {
    return -1;
  }

  /* Set value to max. */
  v->index[v->max] = val;
  v->max++;

  /* Return success. */
  return 0;
}

/* Set value to the vector at the index i. */
int
vector_set_index(struct vector *v, uint32_t i, void *val) {
  int ret;

  ret = vector_ensure(v, i);
  if (! ret) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  v->index[i] = val;
  if (v->max < i) {
    v->max = i;
  }

  return LAGOPUS_RESULT_OK;
}

/* First item of the vector. */
void *
vector_first(struct vector *v) {
  if (v->max == 0) {
    return NULL;
  } else {
    return vector_slot(v, 0);
  }
}

/* Return last element of the vector. */
void *
vector_last(struct vector *v) {
  if (v->max == 0) {
    return NULL;
  }
  return v->index[v->max - 1];
}

/* Unset last element of the vector. */
void *
vector_unset_last(struct vector *v) {
  if (v->max == 0) {
    return NULL;
  }
  v->max--;
  return v->index[v->max];
}

/* Unset vector. */
void
vector_unset(struct vector *v, void *val) {
  vindex_t i;
  size_t size;

  for (i = 0; i < vector_max(v); i++) {
    if (vector_slot(v, i) == val) {
      size = (size_t)(vector_max(v) - (i + 1));
      memmove(v->index + i,
              v->index + (i + 1),
              size * sizeof(void *));
      v->max--;
      return;
    }
  }
}

/* Copy vector. */
struct vector *
vector_dup(struct vector *v) {
  struct vector *vdup;

  vdup = calloc(1, sizeof (struct vector));
  if (vdup == NULL) {
    return NULL;
  }

  vdup->allocated = v->allocated;
  vdup->max = v->max;

  vdup->index = calloc(1, sizeof(void *) * v->allocated);
  if (vdup->index == NULL) {
    free(vdup);
    return NULL;
  }

  memcpy(vdup->index, v->index, sizeof(void *) * v->allocated);

  return vdup;
}

/* Append vector. */
int
vector_append(struct vector *dst, struct vector *src) {
  int ret;

  ret = vector_ensure(dst, dst->max + src->max);
  if (ret < 0) {
    return ret;
  }

  memcpy(dst->index + dst->max, src->index, src->max * sizeof(struct vector *));
  dst->max += src->max;

  return 0;
}

/* Pop last element of the vector. */
void *
vector_pop(struct vector *v) {
  if (v->max == 0) {
    return NULL;
  }
  v->max--;
  return v->index[v->max];
}

