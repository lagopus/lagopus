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





static inline void
s_lock_tssk_q(void) {
  if (likely(s_q_lck != NULL)) {
    (void)lagopus_mutex_lock(&s_q_lck);
  }
}


static inline void
s_unlock_tssk_q(void) {
  if (likely(s_q_lck != NULL)) {
    (void)lagopus_mutex_unlock(&s_q_lck);
  }
}





static inline lagopus_chrono_t
s_schedule_timed_task_no_lock(lagopus_callout_task_t t) {
  lagopus_result_t ret = -1LL;

  if (likely(t != NULL &&
             s_is_task_locked(t) == true &&
             t->m_is_in_q == false)) {
    lagopus_callout_task_t e;

    s_lock_tssk_q();
    {
      for (e = TAILQ_FIRST(&s_chrono_tsk_q);
           e != NULL && e->m_next_abstime <= t->m_next_abstime;
           e = TAILQ_NEXT(e, m_entry)) {
        ;
      }
      if (e != NULL) {
        TAILQ_INSERT_AFTER(&s_chrono_tsk_q, e, t, m_entry);
      } else {
        TAILQ_INSERT_TAIL(&s_chrono_tsk_q, t, m_entry);
      }

      t->m_is_in_q = true;
      (void)s_set_task_state_no_lock(t, TASK_STATE_ENQUEUED);
    }
    s_unlock_tssk_q();

    if (t->m_is_first == false && t->m_do_repeat == true) {
      t->m_next_abstime = t->m_last_abstime + t->m_interval_time;
    } else {
      t->m_next_abstime = t->m_last_abstime + t->m_initial_delay_time;
    }

    ret = t->m_next_abstime;
  }

  return ret;
}


static inline lagopus_chrono_t
s_schedule_timed_task(lagopus_callout_task_t t) {
  lagopus_result_t ret = -1LL;

  if (likely(t != NULL &&
             s_is_task_locked(t) == true &&
             t->m_is_in_q == false)) {
    lagopus_callout_task_t e;

    s_lock_global();
    {

      s_lock_tssk_q();
      {
        for (e = TAILQ_FIRST(&s_chrono_tsk_q);
             e != NULL && e->m_next_abstime <= t->m_next_abstime;
             e = TAILQ_NEXT(e, m_entry)) {
          ;
        }
        if (e != NULL) {
          TAILQ_INSERT_AFTER(&s_chrono_tsk_q, e, t, m_entry);
        } else {
          TAILQ_INSERT_TAIL(&s_chrono_tsk_q, t, m_entry);
        }

        t->m_is_in_q = true;
        (void)s_set_task_state_no_lock(t, TASK_STATE_ENQUEUED);
      }
      s_unlock_tssk_q();

      if (t->m_is_first == false && t->m_do_repeat == true) {
        t->m_next_abstime = t->m_last_abstime + t->m_interval_time;
      } else {
        t->m_next_abstime = t->m_last_abstime + t->m_initial_delay_time;
      }

      ret = t->m_next_abstime;
    }
    s_unlock_global();

  }

  return ret;
}


static inline void
s_unschedule_timed_task_no_lock(lagopus_callout_task_t t) {
  if (likely(t != NULL &&
             s_is_task_locked(t) == true &&
             t->m_is_in_q == true)) {

    s_lock_tssk_q();
    {
      TAILQ_REMOVE(&s_chrono_tsk_q, t, m_entry);
      t->m_is_in_q = false;
      (void)s_set_task_state_no_lock(t, TASK_STATE_DEQUEUED);
    }
    s_unlock_tssk_q();

  }
}


static inline void
s_unschedule_timed_task(lagopus_callout_task_t t) {
  if (likely(t != NULL &&
             s_is_task_locked(t) == true &&
             t->m_is_in_q == true)) {

    s_lock_global();
    {

      s_lock_tssk_q();
      {
        TAILQ_REMOVE(&s_chrono_tsk_q, t, m_entry);
        t->m_is_in_q = false;
        (void)s_set_task_state_no_lock(t, TASK_STATE_DEQUEUED);
      }
      s_unlock_tssk_q();

    }
    s_unlock_global();

  }
}


static inline lagopus_callout_task_t
s_peek_timed_task(void) {
  lagopus_callout_task_t ret = NULL;

  s_lock_tssk_q();
  {
    ret = TAILQ_FIRST(&s_chrono_tsk_q);
  }
  s_unlock_tssk_q();

  return ret;
}


static inline lagopus_callout_task_t
s_get_timed_task_no_lock(void) {
  lagopus_callout_task_t ret = NULL;

  s_lock_tssk_q();
  {
    ret = TAILQ_FIRST(&s_chrono_tsk_q);
    if (likely(ret != NULL)) {
      ret->m_is_in_q = false;
      TAILQ_REMOVE(&s_chrono_tsk_q, ret, m_entry);
      (void)s_set_task_state_no_lock(ret, TASK_STATE_DEQUEUED);
    }
  }
  s_unlock_tssk_q();

  return ret;
}


static inline lagopus_callout_task_t
s_get_timed_task(void) {
  lagopus_callout_task_t ret = NULL;

  s_lock_global();
  {

    s_lock_tssk_q();
    {
      ret = TAILQ_FIRST(&s_chrono_tsk_q);
      if (likely(ret != NULL)) {
        ret->m_is_in_q = false;
        TAILQ_REMOVE(&s_chrono_tsk_q, ret, m_entry);
        (void)s_set_task_state_no_lock(ret, TASK_STATE_DEQUEUED);
      }
    }
    s_unlock_tssk_q();

  }
  s_unlock_global();

  return ret;
}

