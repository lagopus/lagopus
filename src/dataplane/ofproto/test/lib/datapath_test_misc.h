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

#ifndef __DATAPLANE_TEST_MISC_H__
#define __DATAPLANE_TEST_MISC_H__

/*
 * The miscellaneous functions found duplicated across the
 * datapath/dpmgr test sources.  Such the functions should be moved
 * here so that we avoid code duplication.
 *
 * The declarations and definitions should be sorted
 * lexicographically.
 *
 * Macros should be placed in datapath_test_misc_macros.h so that we
 * prevent namespace pollution.
 */

/* Function prototypes. */
void add_match(struct match_list *match_list, uint8_t oxmsize, uint8_t field,
               ...);
struct flow *allocate_test_flow(size_t xs);
void free_test_flow(struct flow *flow);
void refresh_match(struct flow *flow);
void set_match(uint8_t *match, uint8_t oxmsize, uint8_t field, ...);

#endif /* __DATAPLANE_TEST_MISC_H__ */
