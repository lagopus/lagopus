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
