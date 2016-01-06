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
#include "lagopus_thread_internal.h"





#define NTHDS	8





typedef struct a_obj_record {
  lagopus_mutex_t m_lock;
  bool m_bool;
} a_obj_record;
typedef a_obj_record *a_obj_t;


typedef struct null_thread_record {
  lagopus_thread_record m_thd;
  lagopus_mutex_t m_lock;
  a_obj_t m_o;
} null_thread_record;
typedef null_thread_record 	*null_thread_t;





static null_thread_record s_thds[NTHDS];





static inline a_obj_t
a_obj_create(void) {
  a_obj_t ret = (a_obj_t)malloc(sizeof(*ret));

  if (ret != NULL) {
    if (lagopus_mutex_create(&(ret->m_lock)) == LAGOPUS_RESULT_OK) {
      ret->m_bool = false;
    } else {
      free((void *)ret);
      ret = NULL;
    }
  }

  return ret;
}


static inline void
a_obj_destroy(a_obj_t o) {
  lagopus_msg_debug(1, "enter.\n");

  if (o != NULL) {
    if (o->m_lock != NULL) {
      (void)lagopus_mutex_lock(&(o->m_lock));
      (void)lagopus_mutex_unlock(&(o->m_lock));
      (void)lagopus_mutex_destroy(&(o->m_lock));
    }
    free((void *)o);
  }

  lagopus_msg_debug(1, "leave.\n");
}


static inline bool
a_obj_get(a_obj_t o) {
  bool ret = false;

  if (o != NULL) {
    int cstate;

    (void)lagopus_mutex_enter_critical(&(o->m_lock), &cstate);
    {
      ret = o->m_bool;
      /*
       * fprintf() is a cancel point.
       */
      fprintf(stderr, "%s:%d:%s: called.\n",
              __FILE__, __LINE__, __func__);
    }
    (void)lagopus_mutex_leave_critical(&(o->m_lock), cstate);

  }

  return ret;
}


static inline void
a_obj_set(a_obj_t o, bool val) {
  if (o != NULL) {
    int cstate;

    (void)lagopus_mutex_enter_critical(&(o->m_lock), &cstate);
    {
      o->m_bool = val;
      /*
       * fprintf() is a cancel point.
       */
      fprintf(stderr, "%s:%d:%s:0x" PFTIDS(016, x) ": called.\n",
              __FILE__, __LINE__, __func__, pthread_self());
    }
    (void)lagopus_mutex_leave_critical(&(o->m_lock), cstate);

  }
}





static lagopus_result_t
s_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    null_thread_t nptr = (null_thread_t)*tptr;
    a_obj_t o = nptr->m_o;

    if (nptr != NULL &&
        o != NULL) {
      int cstate;

      while (true) {

        (void)lagopus_mutex_enter_critical(&(nptr->m_lock), &cstate);
        {
          /*
           * fprintf() is a cancel point.
           */
          fprintf(stderr, "%s:%d:%s:0x" PFTIDS(016, x) ": sleep one sec.\n",
                  __FILE__, __LINE__, __func__, pthread_self());
          sleep(1);
        }
        (void)lagopus_mutex_leave_critical(&(nptr->m_lock), cstate);

        a_obj_set(o, true);

      }

      ret = LAGOPUS_RESULT_OK;

    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}



static void
s_finalize(const lagopus_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;
  (void)tptr;

  lagopus_msg_debug(1, "called. %s.\n",
                    (is_canceled == false) ? "finished" : "canceled");
}


static inline lagopus_result_t
s_create(null_thread_t *nptr, a_obj_t o) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (nptr != NULL) {
    if ((ret = lagopus_thread_create((lagopus_thread_t *)nptr,
                                     s_main, s_finalize, NULL,
                                     "waiter", NULL)) == LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_mutex_create(&((*nptr)->m_lock))) ==
          LAGOPUS_RESULT_OK) {
        (*nptr)->m_o = o;
      } else {
        lagopus_perror(ret);
        goto done;
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





int
main(int argc, const char *const argv[]) {
  int ret = 1;
  lagopus_result_t r;
  null_thread_t nt;
  size_t i;
  a_obj_t o;

  (void)argc;
  (void)argv;

  o = a_obj_create();
  if (o == NULL) {
    goto done;
  }

  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    if ((r = s_create(&nt, o)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(r);
      goto done;
    }
  }

  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    if ((r = lagopus_thread_start((lagopus_thread_t *)&nt, false)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(r);
      goto done;
    }
  }

  sleep(1);

  /*
   * Cancel all the thread.
   */
  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    if ((r = lagopus_thread_cancel((lagopus_thread_t *)&nt)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(r);
      goto done;
    }
  }

  /*
   * Destroy all the thread. If any deadlocks occur, we failed.
   */
  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    lagopus_thread_destroy((lagopus_thread_t *)&nt);
  }

  /*
   * And destroy a_obj. If a deadlock occurs, we failed.
   */
  a_obj_destroy(o);

  ret = 0;

done:
  return ret;
}
