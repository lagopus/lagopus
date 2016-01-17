/* %COPYRIGHT% */

/**
 *	@file	callback.c
 *	@brief	Callback function management
 */

#include <inttypes.h>

#include "lagopus_types.h"
#include "lagopus/dp_apis.h"
#include "callback.h"

dp_dataq_put_func_t dataq_put_func = NULL;
dp_eventq_put_func_t eventq_put_func = NULL;

dp_dataq_put_func_t
dp_dataq_put_func_register(dp_dataq_put_func_t func) {
  dp_dataq_put_func_t oldfunc;

  oldfunc = dataq_put_func;
  dataq_put_func = func;
  return oldfunc;
}

dp_eventq_put_func_t
dp_eventq_put_func_register(dp_eventq_put_func_t func) {
  dp_eventq_put_func_t oldfunc;

  oldfunc = eventq_put_func;
  eventq_put_func = func;
  return oldfunc;
}

lagopus_result_t
dp_dataq_data_put(uint64_t dpid,
                  struct eventq_data **data,
                  lagopus_chrono_t timeout) {
  if (dataq_put_func != NULL) {
    return dataq_put_func(dpid, data, timeout);
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_eventq_data_put(uint64_t dpid,
                   struct eventq_data **data,
                   lagopus_chrono_t timeout) {
  if (eventq_put_func != NULL) {
    return eventq_put_func(dpid, data, timeout);
  }
  return LAGOPUS_RESULT_OK;
}
