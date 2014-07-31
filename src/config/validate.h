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


#ifndef SRC_CONFIG_VALIDATE_H_
#define SRC_CONFIG_VALIDATE_H_

/* Forward declaration. */
struct cnode;

/* Validation return result. */
enum match_type {
  NONE_MATCH           = 0,
  PARTIAL_MATCH        = 1,
  WORD_MATCH           = 2,
  IPV4_MATCH           = 3,
  IPV4_PREFIX_MATCH    = 4,
  IPV6_MATCH           = 5,
  IPV6_PREFIX_MATCH    = 6,
  MAC_ADDRESS_MATCH    = 7,
  RANGE_MATCH          = 8,
  KEYWORD_MATCH        = 9
};

char *
keyword_match(char *cp, char *name, enum match_type *type);

char *
word_match(char *cp, enum match_type *type);

char *
ipv4_match(char *cp, enum match_type *type);

char *
ipv4_prefix_match(char *cp, enum match_type *type);

char *
ipv6_match(char *cp, enum match_type *type);

char *
ipv6_prefix_match(char *cp, enum match_type *type);

char *
mac_address_match(char *cp, enum match_type *type);

char *
str2int64(char *str, int64_t *val);

char *
range_match(char *cp, struct cnode *cnode, enum match_type *type);

#endif /* SRC_CONFIG_VALIDATE_H_ */
