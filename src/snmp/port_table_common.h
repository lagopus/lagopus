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

#ifndef PORT_TABLE_COMMON_H
#define PORT_TABLE_COMMON_H

#include "ifTable_type.h"

struct port_table_loop_context {
  uint32_t refcnt;
  size_t index;
  int32_t ifIndex;              /* 1 origin */
  struct port_stat *port_stat;  /* port statistics */
};

struct port_table_data_context {
  struct ifTable_entry entry;   /* for temporary data storage */
  size_t index;
  int32_t ifIndex;              /* 1 origin */
  struct port_table_loop_context *lctx;  /* port statistics */
};

#endif /* PORT_TABLE_COMMON_H */
