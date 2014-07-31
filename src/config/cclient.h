/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#ifndef SRC_CONFIG_CCLIENT_H_
#define SRC_CONFIG_CCLIENT_H_

struct cclient;

struct cclient *
cclient_alloc(void);

void
cclient_type_set(struct cclient *cclient, enum confsys_msg_type type);

enum confsys_msg_type
cclient_type_get(struct cclient *cclient);

void
cclient_free(struct cclient *cclient);

void
cclient_stop(struct cclient *cclient);

int
cclient_request(struct cclient *cclient, enum confsys_msg_type type);

int
cclient_request_with_args(struct cclient *cclient, int argc, char **argv);

#endif /* SRC_CONFIG_CCLIENT_H_ */
