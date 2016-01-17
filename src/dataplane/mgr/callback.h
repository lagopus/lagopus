/* %COPYRIGHT% */

/**
 *	@file	callback.h
 *	@brief	Callback function management
 */


lagopus_result_t
dp_dataq_data_put(uint64_t dpid,
                  struct eventq_data **data,
                  lagopus_chrono_t timeout);

lagopus_result_t
dp_eventq_data_put(uint64_t dpid,
                   struct eventq_data **data,
                   lagopus_chrono_t timeout);
