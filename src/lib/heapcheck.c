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





extern int end;





static bool s_is_running_under_valgrind = false;
static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static void s_ctors(void) __attr_constructor__(101);
static void s_dtors(void) __attr_destructor__(101);





#if 0
static void
s_mcheck_proc(enum mcheck_status s) {
  (void)s;
}
#endif


static void
s_once_proc(void) {
  char *a;
  uintptr_t b;

#if 0
  (void)mcheck(s_mcheck_proc);
#endif

  a = (char *)malloc(10);
  b = (uintptr_t)sbrk((intptr_t)0);
  if (b <= (uintptr_t)&end || (uintptr_t)a >= b) {
    s_is_running_under_valgrind = true;
  }
  free((void *)a);
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();
}


static inline void
s_final(void) {
}


static void
s_dtors(void) {
  s_final();
}





static inline bool
s_is_in_heap(const void *addr) {
  if (s_is_running_under_valgrind == false) {
    return ((((uintptr_t)&end) <= ((uintptr_t)addr)) &&
            (((uintptr_t)addr) < ((uintptr_t)sbrk((intptr_t)0)))) ?
           true : false;
  } else {
    lagopus_msg_warning("The malloc()'d address is not in "
                        "sbrk() - end range, "
                        "could be running under the valgrind.\n");
    return false;
  }
}


#if 0
static inline bool
s_is_mallocd(const void *addr) {
  char *caddr0 = (char *)addr;
  char *caddr1 = caddr0 - 1;

  return (mprobe((void *)caddr0) == MCHECK_OK &&
          mprobe((void *)caddr1) == MCHECK_HEAD) ? true : false;
}
#endif





void
lagopus_heapcheck_module_initialize(void) {
  s_init();
}


bool
lagopus_heapcheck_is_in_heap(const void *addr) {
  return s_is_in_heap(addr);
}


#if 0
bool
lagopus_heapcheck_is_mallocd(const void *addr) {
  return s_is_mallocd(addr);
}
#endif
