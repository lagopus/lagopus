/* %COPYRIGHT% */

/**
 *      @file   thread.h
 *      @brief  Dataplane common thread routines.
 */

#ifndef SRC_INCLUDE_DATAPLANE_OFPROTO_THREAD_H_
#define SRC_INCLUDE_DATAPLANE_OFPROTO_THREAD_H_

void
dp_finalproc(const lagopus_thread_t *t, bool is_canceled, void *arg);

void
dp_freeproc(const lagopus_thread_t *t, void *arg);

lagopus_result_t
dp_thread_start(lagopus_thread_t *threadptr,
                lagopus_mutex_t *lockptr,
                bool *runptr);

void dp_thread_finalize(lagopus_thread_t *threadptr);

lagopus_result_t
dp_thread_shutdown(lagopus_thread_t *threadptr,
                   lagopus_mutex_t *lockptr,
                   bool *runptr,
                   shutdown_grace_level_t level);

lagopus_result_t
dp_thread_stop(lagopus_thread_t *threadptr, bool *runptr);

#endif /* SRC_INCLUDE_DATAPLANE_OFPROTO_THREAD_H_ */
