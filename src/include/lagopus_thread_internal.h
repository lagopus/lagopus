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

#ifndef __LAGOPUS_THREAD_INTERNAL_H__
#define __LAGOPUS_THREAD_INTERNAL_H__





typedef struct lagopus_thread_record {
  volatile pthread_t m_pthd;
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  /*
   * For pthread_setaffinity_np(3)
   */
  cpu_set_t *m_cpusetptr;
#else
  /*
   * Dummy.
   */
  void *m_cpusetptr;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
  void *m_arg;

  char m_name[16];	/* max 16 characters. */

  pid_t m_creator_pid;

  lagopus_thread_main_proc_t m_main_proc;
  lagopus_thread_finalize_proc_t m_final_proc;
  lagopus_thread_freeup_proc_t m_freeup_proc;

  lagopus_mutex_t m_op_lock;
  lagopus_mutex_t m_wait_lock;
  lagopus_mutex_t m_cancel_lock;
  lagopus_mutex_t m_finalize_lock;

  lagopus_cond_t m_wait_cond;
  lagopus_cond_t m_startup_cond;
  lagopus_cond_t m_finalize_cond;

  lagopus_result_t m_result_code;

  volatile bool m_is_started;
  volatile bool m_is_activated;
  volatile bool m_is_canceled;
  volatile bool m_is_finalized;
  volatile bool m_is_destroying;

  volatile bool m_do_autodelete;
  volatile bool m_startup_sync_done;
  volatile size_t m_n_finalized_count;

} lagopus_thread_record;





#endif /* ! __LAGOPUS_THREAD_INTERNAL_H__ */
