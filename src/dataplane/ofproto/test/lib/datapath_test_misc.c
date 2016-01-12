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

/*
 * The miscellaneous functions found duplicated across the
 * datapath/dpmgr test sources.  Such the functions should be moved
 * here so that we avoid code duplication.
 *
 * The declarations and definitions should be sorted
 * lexicographically.
 */

#include <sys/types.h>

#include <stdarg.h>

#include "lagopus/flowdb.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


void
add_match(struct match_list *match_list, uint8_t oxmsize, uint8_t field,
          ...) {
  va_list ap;
  struct match *match;
  int i;

  match = (struct match *)calloc(1, sizeof(struct match) + oxmsize);
  match->oxm_class = OFPXMC_OPENFLOW_BASIC;
  match->oxm_field = field;
  match->oxm_length = oxmsize;
  va_start(ap, field);
  for (i = 0; i < oxmsize; i++) {
    match->oxm_value[i] = (uint8_t)va_arg(ap, int);
  }
  va_end(ap);
  /* Add to match list. */
  TAILQ_INSERT_TAIL(match_list, match, entry);
}

/*
 * @brief Allocate a new flow for testing.
 * @param[in] xs The extra allocation size, added to sizeof(struct flow).
 * @retval The pointer to a new flow, or NULL upon an error.
 */
struct flow *
allocate_test_flow(size_t xs) {
  struct flow *flow;

  flow = calloc(1, sizeof(struct flow) + xs);
  TAILQ_INIT(&flow->match_list);
  TAILQ_INIT(&flow->instruction_list);

  return flow;
}

/*
 * @brief Free flow for testing.
 * @param[in] flow The pointer to a test flow.
 * @retval None.
 */
void
free_test_flow(struct flow *flow) {
  free(flow);
}

void
refresh_match(struct flow *flow) {
  flow_make_match(flow);
#if 0
  struct match *match;
  int i;

  i = 0;
  TAILQ_FOREACH(match, &flow->match_list, entry) {
    if (match->except_flag == true) {
      continue;
    }
    flow->match[i++] = match;
  }
  flow->match[i] = NULL;
#endif
}

void
set_match(uint8_t *match, uint8_t oxmsize, uint8_t field, ...) {
  va_list ap;
  int i;

  match[0] = NBO_BYTE(OFPXMC_OPENFLOW_BASIC, 0);
  match[1] = NBO_BYTE(OFPXMC_OPENFLOW_BASIC, 1);
  match[2] = field;
  match[3] = oxmsize;
  va_start(ap, field);
  for (i = 0; i < oxmsize; i++) {
    match[4 + i] = (uint8_t)va_arg(ap, int);
  }
  va_end(ap);
}
