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


#ifndef __VECTOR_H__
#define __VECTOR_H__

/* Typedef vector index as vindex_t. */
typedef uint32_t      vindex_t;

/* Vector structure. */
struct vector {
  /* Max sequential used slot. */
  vindex_t max;

  /* Number of allocated slot. */
  vindex_t allocated;

  /* Index to the data. */
  void **index;
};

#define VECTOR_MIN_SIZE 8

struct vector *
vector_alloc(void);

void
vector_free(struct vector *v);

int
vector_set(struct vector *v, void *val);

int
vector_set_index(struct vector *v, vindex_t i, void *val);

void
vector_wrapper_only_free(struct vector *v);

void *
vector_first(struct vector *v);

void *
vector_last(struct vector *v);

void
vector_unset(struct vector *v, void *val);

void *
vector_unset_last(struct vector *v);

vindex_t
vector_max(struct vector *v);

void *
vector_slot(struct vector *v, vindex_t i);

void
vector_reset(struct vector *v);

struct vector *
vector_dup(struct vector *v);

int
vector_append(struct vector *dst, struct vector *src);

void *
vector_pop(struct vector *v);

#endif /* __VECTOR_H__ */
