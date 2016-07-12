

static inline void
s_legacy_sw_stage_freeup(const lagopus_pipeline_stage_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    legacy_sw_stage_t ts = (legacy_sw_stage_t)(*sptr);

    if (ts->m_lock != NULL) {
      lagopus_mutex_destroy(&(ts->m_lock));
    }
    if (ts->m_cond != NULL) {
      lagopus_cond_destroy(&(ts->m_cond));
    }
  }
}


static inline lagopus_result_t
s_legacy_sw_stage_create(legacy_sw_stage_t *tsptr,
                    const char *name,
                    size_t n_workers,
                    size_t n_qs,
                    size_t q_len,
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
             n_workers > 0 && max_stage > 0)) {
    lagopus_mutex_t lock = NULL;
    lagopus_cond_t cond = NULL;
    lagopus_pipeline_stage_throw_proc_t throw_proc = (stage_idx == max_stage -1) ? NULL : s_base_throw;

    if (likely((ret = lagopus_mutex_create(&lock)) == LAGOPUS_RESULT_OK &&
               (ret = lagopus_cond_create(&cond)) == LAGOPUS_RESULT_OK &&
               (ret = s_base_create((base_stage_t *)tsptr,
                                    sizeof(legacy_sw_stage_record),
                                    name,       /* pipeline name */
                                    stage_idx,	/* stage_idx */
                                    max_stage,	/* max_stage */
                                    n_workers,	/* n_workers */
                                    n_qs,	/* n_qs */
                                    q_len,	/* q_len */
                                    batch_size,	/* batch_size */
                                    to,		/* to */
                                    sched_proc,		/* sched_proc */
                                    fetch_proc,		/* fetch_proc */
                                    main_proc,		/* main_proc */
                                    throw_proc,		/* throw_proc */
                                    s_legacy_sw_stage_freeup	/* freeup_proc */
                                    )) == LAGOPUS_RESULT_OK)) {

      (*tsptr)->m_lock = lock;
      (*tsptr)->m_cond = cond;
      (*tsptr)->m_do_exit = false;

      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_legacy_sw_stage_wait(const legacy_sw_stage_t ts) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(ts->m_lock != NULL && ts->m_cond != NULL)) {

    (void)lagopus_mutex_lock(&(ts->m_lock));
    {
   recheck:
      if (ts->m_do_exit != true) {
        (void)lagopus_cond_wait(&(ts->m_cond), &(ts->m_lock), -1LL);
        goto recheck;
      }
    }
    (void)lagopus_mutex_unlock(&(ts->m_lock));

    ret = LAGOPUS_RESULT_OK;

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline lagopus_result_t
s_legacy_sw_stage_wakeup(const legacy_sw_stage_t ts) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(ts->m_lock != NULL && ts->m_cond != NULL)) {

    (void)lagopus_mutex_lock(&(ts->m_lock));
    {
      ts->m_do_exit = true;
      (void)lagopus_cond_notify(&(ts->m_cond), true);
    }
    (void)lagopus_mutex_unlock(&(ts->m_lock));

    ret = LAGOPUS_RESULT_OK;

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_stage_create(legacy_sw_stage_t *tsptr,
               const char *name,
               size_t n_workers,
               size_t n_qs,
               size_t q_len,
               size_t batch_size,
               lagopus_chrono_t to,
               lagopus_pipeline_stage_sched_proc_t sched_proc,
               lagopus_pipeline_stage_fetch_proc_t fetch_proc,
               lagopus_pipeline_stage_main_proc_t main_proc,
               size_t stage_idx,
               size_t max_stage) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL &&
             n_workers > 0 && n_qs > 0 &&
             q_len > 0 && batch_size > 0 &&
             sched_proc != NULL &&
             fetch_proc != NULL &&
             stage_idx < max_stage &&
             max_stage > 0)) {
    ret = s_legacy_sw_stage_create(tsptr,
                              name,	        /* name */
                              n_workers,	/* n_workers */
                              n_qs,		/* n_qs */
                              q_len,		/* q_len */
                              batch_size,	/* batch_size */
                              to,		/* to */
                              sched_proc,	/* sched_proc */
                              fetch_proc,	/* fetch_proc */
                              main_proc,	/* main_proc */
                              stage_idx,	/* stage_idx */
                              max_stage		/* max_stage */
                              );
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline lagopus_result_t
s_legacy_sw_stage_create_by_spec(legacy_sw_stage_t *tsptr,
                            const char *name,
                            size_t stg_idx,
                            size_t max_stage,
                            legacy_sw_stage_spec_t *spec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (likely(spec != NULL &&
             spec->m_n_workers > 0 &&
             stg_idx < max_stage &&
             spec->m_n_qs > 0 &&
		         spec->m_q_len > 0 &&
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
    ret = s_stage_create(tsptr,
                         name,
                         spec->m_n_workers,
                         spec->m_n_qs,
                         spec->m_q_len,
                         spec->m_batch_size,
                         spec->m_to,
                         sched_proc,
                         fetch_proc,
                         spec->main_proc,
                         stg_idx,
                         max_stage);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}
