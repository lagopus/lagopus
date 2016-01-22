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
 * @file	mactable_cmd.h
 */

#ifndef __LAGOPUS_DATASTORE_MACTABLE_CMD_H__
#define __LAGOPUS_DATASTORE_MACTABLE_CMD_H__

typedef struct datastore_macentry {
  uint64_t mac_addr;
  uint32_t port_no;
  uint32_t update_time;
  uint16_t address_type;
} datastore_macentry_t;

#endif /* __MACTABLE_CMD_H__ */
