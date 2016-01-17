/* %COPYRIGHT% */

#ifndef SRC_AGENT_OFP_DPQUEUE_MGR_H_
#define SRC_AGENT_OFP_DPQUEUE_MGR_H_

lagopus_result_t
ofp_dpqueue_mgr_initialize(int argc,
                           const char *const argv[],
                           void *extarg,
                           lagopus_thread_t **thdptr);
lagopus_result_t ofp_dpqueue_mgr_start(void);
void ofp_dpqueue_mgr_finalize(void);
lagopus_result_t ofp_dpqueue_mgr_shutdown(shutdown_grace_level_t level);
lagopus_result_t ofp_dpqueue_mgr_stop(void);

#endif /* SRC_AGENT_OFP_DPQUEUE_MGR_H_ */
