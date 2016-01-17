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
