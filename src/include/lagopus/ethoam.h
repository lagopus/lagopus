/**
 * @file  ethoam.c
 * @brief  ethoam APIs.
 * @details  create/destroy ethoam thread.
 */
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

#ifndef SRC_LAGOPUS_ETHOAM_H_
#define SRC_LAGOPUS_ETHOAM_H_

//#include <stdio.h>
//#include <Python.h>

#include "lagopus_apis.h"
#include "lagopus_gstate.h"
#include "lagopus_thread_internal.h"

/**
 * @brief ethoam_initialize
 * @details initilaizing ethoam
 * @version
 * @param [in] arg arg from command line
 * @param [out] thdptr thread pointer
 * @return LAGOPUS_RESULT_OK initialized successfully.
 * @return !=LAGOPUS_RESULT_OK failed to initialize.
 * @exception -
 * @see -
 * @date 2014/02/28 VSW-1 Akinari Kumazawa New
 */
lagopus_result_t ethoam_initialize(void *arg, lagopus_thread_t **thdptr);

/**
 * @brief ethoam_start
 * @details ethoam start function
 * @version
 * @param -
 * @return LAGOPUS_RESULT_OK initialized successfully.
 * @return !=LAGOPUS_RESULT_OK failed to initialize.
 * @exception -
 * @see -
 * @date 2014/02/28 VSW-1 Akinari Kumazawa New
 */
lagopus_result_t ethoam_start(void);

/**
 * @brief ethoam_shutdown
 * @details ethoam shutdown function
 * @version
 * @param [in] level shutdown grace level
 * @return LAGOPUS_RESULT_OK initialized successfully.
 * @return !=LAGOPUS_RESULT_OK failed to initialize.
 * @exception -
 * @see -
 * @date 2014/02/28 VSW-1 Akinari Kumazawa New
 */
lagopus_result_t ethoam_shutdown(shutdown_grace_level_t level);

/**
 * @brief ethoam_finalize
 * @details finalize ethoam
 * @version
 * @param -
 * @return -
 * @exception -
 * @see -
 * @date 2014/02/28 VSW-1 Akinari Kumazawa New
 */
void ethoam_finalize(void);

#endif /* SRC_LAGOPUS_ETHOAM_H_ */

