#ifndef __LAGOPUS_RUNNABLE_INTERNAL_H__
#define __LAGOPUS_RUNNABLE_INTERNAL_H__





#include "lagopus_runnable_funcs.h"





typedef struct lagopus_runnable_record {
  lagopus_runnable_proc_t m_func;
  void *m_arg;
  lagopus_runnable_freeup_proc_t m_freeup_func;
  bool m_is_heap_allocd;
} lagopus_runnable_record;





#endif /* ! __LAGOPUS_RUNNABLE_INTERNAL_H__ */
