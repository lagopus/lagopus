/* %COPYRIGHT% */

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
