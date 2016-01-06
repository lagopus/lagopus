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

#ifndef __DATASTORE_CONFIG_H__
#define __DATASTORE_CONFIG_H__




/**
 *	@file	datastore_config.h
 */





/**
 * Set initial configuration file.
 *
 *	@param[in]	file	A name of a configuration file.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 */
lagopus_result_t
datastore_set_config_file(const char *file);


/**
 * Get initial configuration file.
 *
 *	@param[out]	file	A pointer to the returned configuration file.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 */
lagopus_result_t
datastore_get_config_file(const char **file);


/**
 * Pre-load the configuration.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This API must be called BEFORE all the lagopus
 *	modules are initialized sicen the API is for loading
 *	default/mandatory parameters needed to initialize the module.
 */
lagopus_result_t
datastore_preload_config(void);


/**
 * Load the configuration.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval LAGOPUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details After all the lagopus modules are initialized, call
 *	this API to load the full configuration parameters.
 */
lagopus_result_t
datastore_load_config(void);





#endif /* ! __DATASTORE_CONFIG_H__ */
