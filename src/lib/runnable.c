#include "lagopus_apis.h"
#include "lagopus_runnable_internal.h"





#define DEFAULT_RUNNABLE_ALLOC_SZ	(sizeof(lagopus_runnable_record))





lagopus_result_t
lagopus_runnable_create(lagopus_runnable_t *rptr,
                        size_t alloc_sz,
                        lagopus_runnable_proc_t func,
                        void *arg,
                        lagopus_runnable_freeup_proc_t freeup_func) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_heap_allocd = false;

  if (rptr != NULL &&
      func != NULL) {
    if (*rptr == NULL) {
      size_t sz = (DEFAULT_RUNNABLE_ALLOC_SZ > alloc_sz) ?
                  DEFAULT_RUNNABLE_ALLOC_SZ : alloc_sz;
      *rptr = (lagopus_runnable_t)malloc(sz);
      if (*rptr != NULL) {
        is_heap_allocd = true;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto done;
      }
    }

    (*rptr)->m_func = func;
    (*rptr)->m_arg = arg;
    (*rptr)->m_freeup_func = freeup_func;
    (*rptr)->m_is_heap_allocd = is_heap_allocd;

    ret = LAGOPUS_RESULT_OK;

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  if (ret != LAGOPUS_RESULT_OK) {
    if (is_heap_allocd == true) {
      free((void *)(*rptr));
    }
    if (rptr != NULL) {
      *rptr = NULL;
    }
  }

  return ret;
}


void
lagopus_runnable_destroy(lagopus_runnable_t *rptr) {
  if (rptr != NULL) {
    if ((*rptr)->m_freeup_func != NULL) {
      ((*rptr)->m_freeup_func)(rptr);
    }
    if ((*rptr)->m_is_heap_allocd == true) {
      (void)free((void *)(*rptr));
    }
    *rptr = NULL;
  }
}


lagopus_result_t
lagopus_runnable_start(const lagopus_runnable_t *rptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (rptr != NULL && *rptr != NULL) {
    ret = ((*rptr)->m_func)(rptr, (*rptr)->m_arg);
  }

  return ret;
}
