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

#ifndef __LAGOPUS_DATASTORE_COMMON_H__
#define __LAGOPUS_DATASTORE_COMMON_H__

/**
 * @file	datastore/common.h
 */

#include "lagopus_apis.h"

#define DATASTORE_NAMESPACE_DELIMITER ":"
#define DATASTORE_NAMESPACE_DELIMITER_LEN 1

#define DATASTORE_DRYRUN_NAMESPACE "dryrun"

#define DATASTORE_NAMESPACE_MAX 256
#define DATASTORE_CHANNEL_NAME_MAX 512
#define DATASTORE_CHANNEL_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_CHANNEL_NAME_MAX)
#define DATASTORE_CONTROLLER_NAME_MAX 512
#define DATASTORE_CONTROLLER_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_CONTROLLER_NAME_MAX)
#define DATASTORE_PORT_NAME_MAX 512
#define DATASTORE_PORT_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_PORT_NAME_MAX)
#define DATASTORE_INTERFACE_NAME_MAX 512
#define DATASTORE_INTERFACE_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_INTERFACE_NAME_MAX)
#define DATASTORE_BRIDGE_NAME_MAX 512
#define DATASTORE_BRIDGE_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_BRIDGE_NAME_MAX)
#define DATASTORE_L2_BRIDGE_NAME_MAX 512
#define DATASTORE_L2_BRIDGE_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_L2_BRIDGE_NAME_MAX)
#define DATASTORE_QUEUE_NAME_MAX 512
#define DATASTORE_QUEUE_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_QUEUE_NAME_MAX)
#define DATASTORE_POLICER_NAME_MAX 512
#define DATASTORE_POLICER_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_POLICER_NAME_MAX)
#define DATASTORE_POLICER_ACTION_NAME_MAX 512
#define DATASTORE_POLICER_ACTION_FULLNAME_MAX \
          (DATASTORE_NAMESPACE_MAX + \
           DATASTORE_NAMESPACE_DELIMITER_LEN + \
           DATASTORE_POLICER_ACTION_NAME_MAX)
#define DATASTORE_CONTROLLER_MAX 1024
#define DATASTORE_PORT_MAX 1024
#define DATASTORE_QUEUE_MAX 1024

struct datastore_name_entry {
  TAILQ_ENTRY(datastore_name_entry) name_entries;
  char *str;
};

TAILQ_HEAD(datastore_name_head, datastore_name_entry);

struct datastore_name_list {
  size_t size;
  struct datastore_name_head head;
};

typedef struct datastore_name_list datastore_name_info_t;

lagopus_result_t
datastore_names_destroy(datastore_name_info_t *names);

lagopus_result_t
datastore_names_create(datastore_name_info_t **names);

lagopus_result_t
datastore_names_duplicate(const datastore_name_info_t *src_names,
                          datastore_name_info_t **dst_names,
                          const char *namespace);

bool
datastore_name_exists(const datastore_name_info_t *names,
                      const char *name);

bool
datastore_names_equals(const datastore_name_info_t *names0,
                       const datastore_name_info_t *names1);

lagopus_result_t
datastore_add_names(datastore_name_info_t *names,
                    const char *name);

lagopus_result_t
datastore_remove_names(datastore_name_info_t *names,
                       const char *name);

lagopus_result_t
datastore_remove_all_names(datastore_name_info_t *names);

#endif /* ! __LAGOPUS_DATASTORE_COMMON_H__ */
