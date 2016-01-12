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

#ifndef __LAGOPUS_DATASTORE_AFFINITION_H__
#define __LAGOPUS_DATASTORE_AFFINITION_H__

#include "common.h"

typedef enum datastore_affinition_info_type {
  DATASTORE_AFFINITION_INFO_TYPE_UNKNOWN = 0,
  DATASTORE_AFFINITION_INFO_TYPE_WORKER,
  DATASTORE_AFFINITION_INFO_TYPE_IO,

  DATASTORE_AFFINITION_INFO_TYPE_MAX,
} datastore_affinition_info_type_t;

/**
 * @brief	datastore_affinition_interface
 */
typedef struct datastore_affinition_interface {
  TAILQ_ENTRY(datastore_affinition_interface) entry;
  char *name;
} datastore_affinition_interface_t;

/**
 * @brief	datastore_affinition_interface_list_t
 */
TAILQ_HEAD(datastore_affinition_interface_list,
           datastore_affinition_interface);
typedef struct datastore_affinition_interface_list
  datastore_affinition_interface_list_t;

/**
 * @brief	datastore_affinition_info_t
 */
typedef struct datastore_affinition_info {
  TAILQ_ENTRY(datastore_affinition_info) entry;
  uint64_t core_no;
  datastore_affinition_info_type_t type;
  datastore_affinition_interface_list_t interfaces;
} datastore_affinition_info_t;

/**
 * @brief	datastore_affinition_info_t
 */
TAILQ_HEAD(datastore_affinition_info_list,
           datastore_affinition_info);
typedef struct datastore_affinition_info_list
  datastore_affinition_info_list_t;

#endif /* ! __LAGOPUS_DATASTORE_AFFINITION_H__ */
