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
 *	@file	qsort.c
 *	@brief	Thread safe qsort function slimilar to glibc qsort_r.
 */

#include <stdlib.h>

#include "lagopus_qsort.h"

static __thread struct qsort_data {
  int (*cmp)(const void *, const void *, void *);
  void *arg;
} qdata;

static int
qsort_cmp(const void *a, const void *b) {
  return qdata.cmp(a, b, qdata.arg);
}

void
lagopus_qsort_r(void *array,
                size_t nelem,
                size_t size,
                int (*cmp)(const void *, const void *, void *),
                void *arg) {
  qdata.cmp = cmp;
  qdata.arg = arg;
  qsort(array, nelem, size, qsort_cmp);
}
