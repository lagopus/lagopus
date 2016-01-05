static lagopus_result_t	s_ingress_setup(base_stage_t bs);





static inline void
s_test_stage_freeup(const lagopus_pipeline_stage_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    test_stage_t ts = (test_stage_t)(*sptr);

    if (ts->m_lock != NULL) {
      lagopus_mutex_destroy(&(ts->m_lock));
    }
    if (ts->m_cond != NULL) {
      lagopus_cond_destroy(&(ts->m_cond));
    }

    free((void *)ts->m_data);
    free((void *)ts->m_enq_infos);
    free((void *)ts->m_states);
  }
}


static inline lagopus_result_t
s_test_stage_create(test_stage_t *tsptr,
                    test_stage_type_t type,
                    size_t n_workers,
                    size_t n_qs,
                    size_t n_events,
                    size_t batch_size,
                    lagopus_chrono_t to,
                    lagopus_pipeline_stage_sched_proc_t sched_proc,
                    lagopus_pipeline_stage_fetch_proc_t fetch_proc,
                    lagopus_pipeline_stage_main_proc_t main_proc,
                    size_t stage_idx,
                    size_t max_stage) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL && 
             main_proc != NULL &&
             n_workers > 0 && n_events > 0 && max_stage > 0)) {
    lagopus_mutex_t lock = NULL;
    lagopus_cond_t cond = NULL;
    test_stage_state_t *states = (test_stage_state_t *)
        malloc(sizeof(test_stage_state_t) * n_workers);
    lagopus_pipeline_stage_throw_proc_t throw_proc = 
        (type == test_stage_type_intermediate) ? s_base_throw : NULL;

    if (unlikely(states == NULL)) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      goto done;
    }

    if (likely((ret = lagopus_mutex_create(&lock)) == LAGOPUS_RESULT_OK &&
               (ret = lagopus_cond_create(&cond)) == LAGOPUS_RESULT_OK &&
               (ret = s_base_create((base_stage_t *)tsptr,
                                    "test",
                                    stage_idx,	/* stage_idx */
                                    max_stage,	/* max_stage */
                                    n_workers,	/* n_workers */
                                    n_qs,	/* n_qs */
                                    batch_size,	/* batch_size */
                                    to,		/* to */
                                    sched_proc,		/* sched_proc */
                                    fetch_proc,		/* fetch_proc */
                                    main_proc,		/* main_proc */
                                    throw_proc,		/* throw_proc */
                                    s_test_stage_freeup	/* freeup_proc */
                                    )) == LAGOPUS_RESULT_OK &&
               ((type == test_stage_type_ingress ?
                 (ret = s_base_stage_set_setup_hook((base_stage_t)(*tsptr),
                                                    s_ingress_setup)) :
                 (ret = LAGOPUS_RESULT_OK))) == LAGOPUS_RESULT_OK)) {
      size_t i;

      for (i = 0; i < n_workers; i++) {
        states[i] = test_stage_state_initialized;
      }

      (*tsptr)->m_type = type;
      (*tsptr)->m_lock = lock;
      (*tsptr)->m_cond = cond;
      (*tsptr)->m_do_exit = false;
      (*tsptr)->m_data = NULL;
      (*tsptr)->m_n_data = n_events;
      (*tsptr)->m_enq_infos = NULL;
      (*tsptr)->m_states = states;
      (*tsptr)->m_sum = 0;
      (*tsptr)->m_n_events = 0;
      (*tsptr)->m_start_clock = 0;
      (*tsptr)->m_end_clock = 0;

      ret = LAGOPUS_RESULT_OK;
    } else {
      free((void *)states);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





static lagopus_result_t
s_ingress_setup(base_stage_t bs) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(bs != NULL)) {
    test_stage_t ts = (test_stage_t)bs;
    lagopus_result_t n_workers = lagopus_pipeline_stage_get_worker_nubmer(
        (lagopus_pipeline_stage_t *)&ts);
    size_t n_data = ts->m_n_data;

    if (likely(n_data > 0 &&
               n_workers > 0)) {
      uint64_t *data =
          (uint64_t *)malloc(sizeof(lagopus_chrono_t) * n_data);
      enqueue_info_t *enq_infos = 
          (enqueue_info_t *)malloc(sizeof(enqueue_info_t) * (size_t)n_workers);

      if (likely(data != NULL && enq_infos != NULL)) {
        size_t i;
        size_t rem_size = n_data;
        size_t len = n_data / (size_t)n_workers;

        for (i = 0; i < n_data; i++) {
          data[i] = i;
        }

        if (n_data % (size_t)n_workers != 0) {
          len++;
        }

        for (i = 0; i < (size_t)n_workers; i++) {
          enq_infos[i].m_offset = i * len;
          rem_size -= len;
          enq_infos[i].m_offset = (rem_size < len) ? rem_size : len;
        }

        ts->m_data = data;
        ts->m_enq_infos = enq_infos;

        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static lagopus_result_t
s_ingress_main(const lagopus_pipeline_stage_t *sptr,
               size_t idx,
               void *evbuf,
               size_t n_evs) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)evbuf;
  (void)n_evs;

  if (likely(sptr != NULL && *sptr != NULL)) {
    test_stage_t ts = (test_stage_t)(*sptr);
    base_stage_t bs = (base_stage_t)ts;

    if (likely(bs != NULL && bs->m_next_stg != NULL)) {
      if (likely(ts->m_states[idx] != test_stage_state_done)) {
        uint64_t start_clock;
        uint64_t end_clock;
        uint64_t *addr = ts->m_data + ts->m_enq_infos[idx].m_offset;
        size_t len = ts->m_enq_infos[idx].m_length;

        start_clock = lagopus_rdtsc();
        ret = lagopus_pipeline_stage_submit((lagopus_pipeline_stage_t *)
                                            &(bs->m_next_stg),
                                            (void *)addr, len, (void *)idx);
        end_clock = lagopus_rdtsc();

        lagopus_atomic_update_min(uint64_t, &(ts->m_start_clock),
                                  0, start_clock);
        lagopus_atomic_update_max(uint64_t, &(ts->m_end_clock),
                                  0, end_clock);
        
        ts->m_states[idx] = test_stage_state_done;
        mbar();
      } else {
        if (likely(ts->m_lock != NULL && ts->m_cond)) {

          (void)lagopus_mutex_lock(&(ts->m_lock));
          {
         recheck:
            if (ts->m_do_exit != true) {
              (void)lagopus_cond_wait(&(ts->m_cond), &(ts->m_lock), -1LL);
              goto recheck;
            }
          }
          (void)lagopus_mutex_unlock(&(ts->m_lock));

          ret = 0;
        } else {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_ingress_create(test_stage_t *tsptr,
                 size_t n_workers,
                 size_t n_events,
                 size_t max_stage) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL && n_workers > 0 && n_events > 0 &&
             max_stage > 0)) {
    ret = s_test_stage_create(tsptr,
                              test_stage_type_ingress,
                              n_workers,	/* n_workers */
                              0,		/* n_qs */
                              n_events,		/* n_events */
                              1,		/* batch_size (dummy) */
                              0,		/* to */
                              NULL,		/* sched_proc */
                              NULL,		/* fetch_proc */
                              s_ingress_main,	/* main_proc */
                              0,		/* stage_idx */
                              max_stage		/* max_stage */
                              );
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static lagopus_result_t
s_intermediate_main(const lagopus_pipeline_stage_t *sptr,
                    size_t idx,
                    void *evbuf,
                    size_t n_evs) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    test_stage_t ts = (test_stage_t)(*sptr);
    size_t n_cur_evs = __sync_add_and_fetch(&(ts->m_n_events), 0);

    if (likely(n_cur_evs < ts->m_n_data)) {
      uint64_t *data = (uint64_t *)evbuf;
      size_t i;
      uint64_t sum = 0;

      if (unlikely(ts->m_states[idx] == test_stage_state_initialized)) {
        uint64_t cur_clock = lagopus_rdtsc();
        lagopus_atomic_update_min(uint64_t, &(ts->m_start_clock),
                                  0, cur_clock);
      }

      for (i = 0; i < n_evs; i++) {
        sum += data[i];
      }
      (void)__sync_fetch_and_add(&(ts->m_sum), sum);

      if (unlikely(ts->m_n_data == n_cur_evs)) {
        uint64_t cur_clock = lagopus_rdtsc();
        lagopus_atomic_update_min(uint64_t, &(ts->m_end_clock),
                                  0, cur_clock);
      }

      ret = (lagopus_result_t)n_evs;
    } else {
      ts->m_states[idx] = test_stage_state_done;
      mbar();

      if (likely(ts->m_lock != NULL && ts->m_cond)) {

        (void)lagopus_mutex_lock(&(ts->m_lock));
        {
       recheck:
          if (ts->m_do_exit != true) {
            (void)lagopus_cond_wait(&(ts->m_cond), &(ts->m_lock), -1LL);
            goto recheck;
          }
        }
        (void)lagopus_mutex_unlock(&(ts->m_lock));

        ret = 0;
      } else {
        ret = LAGOPUS_RESULT_INVALID_ARGS;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_intermediate_create(test_stage_t *tsptr,
                      size_t n_workers,
                      size_t n_qs,
                      size_t n_events,
                      size_t batch_size,
                      lagopus_chrono_t to,
                      lagopus_pipeline_stage_sched_proc_t sched_proc,
                      lagopus_pipeline_stage_fetch_proc_t fetch_proc,
                      size_t stage_idx,
                      size_t max_stage) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL && 
             n_workers > 0 && n_qs > 0 && n_events > 0 &&
             batch_size > 0 &&
             sched_proc != NULL &&
             fetch_proc != NULL &&
             stage_idx > 0 && stage_idx < max_stage &&
             max_stage > 0)) {
    ret = s_test_stage_create(tsptr,
                              test_stage_type_intermediate,
                              n_workers,	/* n_workers */
                              n_qs,		/* n_qs */
                              n_events,		/* n_events */
                              batch_size,	/* batch_size */
                              to,		/* to */
                              sched_proc,	/* sched_proc */
                              fetch_proc,	/* fetch_proc */
                              s_intermediate_main,	/* main_proc */
                              stage_idx,	/* stage_idx */
                              max_stage		/* max_stage */
                              );
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_egress_create(test_stage_t *tsptr,
                size_t n_workers,
                size_t n_qs,
                size_t n_events,
                size_t batch_size,
                lagopus_chrono_t to,
                lagopus_pipeline_stage_sched_proc_t sched_proc,
                lagopus_pipeline_stage_fetch_proc_t fetch_proc,
                size_t max_stage) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL && 
             n_workers > 0 && n_qs > 0 && n_events > 0 &&
             batch_size > 0 &&
             sched_proc != NULL &&
             fetch_proc != NULL &&
             max_stage > 0)) {
    ret = s_test_stage_create(tsptr,
                              test_stage_type_egress,
                              n_workers,	/* n_workers */
                              n_qs,		/* n_qs */
                              n_events,		/* n_events */
                              batch_size,	/* batch_size */
                              to,		/* to */
                              sched_proc,	/* sched_proc */
                              fetch_proc,	/* fetch_proc */
                              s_intermediate_main,	/* main_proc */
                              max_stage - 1,	/* stage_idx */
                              max_stage		/* max_stage */
                              );
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_test_stage_create_by_spec(test_stage_t *tsptr,
                            size_t stg_idx,
                            size_t max_stage,
                            test_stage_spec_t *spec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(spec != NULL &&
             spec->m_n_workers > 0 &&
             spec->m_n_events > 0 &&
             stg_idx < max_stage)) {
    if (stg_idx == 0) {
      ret = s_ingress_create(tsptr,
                             spec->m_n_workers,
                             spec->m_n_events,
                             max_stage);
    } else {
      if (likely(spec->m_n_qs > 0 &&
                 spec->m_batch_size > 0)) {
        lagopus_pipeline_stage_sched_proc_t sched_proc = NULL;
        lagopus_pipeline_stage_fetch_proc_t fetch_proc = NULL;

        if (unlikely(spec->m_n_qs == 1 &&
                     (spec->m_sched_type != base_stage_sched_single ||
                      spec->m_fetch_type != base_stage_fetch_single))) {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
          goto done;
        }

        sched_proc = s_base_stage_get_sched_proc(spec->m_sched_type);
        fetch_proc = s_base_stage_get_fetch_proc(spec->m_fetch_type);
        if (unlikely(sched_proc == NULL || fetch_proc == NULL)) {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
          goto done;
        }

        if (stg_idx == (max_stage - 1)) {
          ret = s_egress_create(tsptr,
                                spec->m_n_workers,
                                spec->m_n_qs,
                                spec->m_n_events,
                                spec->m_batch_size,
                                spec->m_to,
                                sched_proc,
                                fetch_proc,
                                max_stage);
        } else {
          ret = s_intermediate_create(tsptr,
                                      spec->m_n_workers,
                                      spec->m_n_qs,
                                      spec->m_n_events,
                                      spec->m_batch_size,
                                      spec->m_to,
                                      sched_proc,
                                      fetch_proc,
                                      stg_idx,
                                      max_stage);
        }
      } else {
        ret = LAGOPUS_RESULT_INVALID_ARGS;
      }
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}
