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

#ifndef __TEST_LIB_UTIL_H__
#define __TEST_LIB_UTIL_H__

#ifndef LOOP_MAX
#define LOOP_MAX 100
#endif /* LOOP_MAX */

#ifndef SLEEP
#define SLEEP usleep(0.1 * 1000 * 1000)
#endif /* SLEEP */

#define TEST_ASSERT_COUNTER(b, c, msg) {                              \
    int _i;                                                           \
    for (_i= 0; _i < LOOP_MAX; _i++) {                                \
      bool _ret = check_called_func_count(c);                         \
      if (_ret == b) {                                                \
        break;                                                        \
      }                                                               \
      SLEEP;                                                          \
    }                                                                 \
    if (_i == LOOP_MAX) {                                             \
      TEST_FAIL_MESSAGE(msg);                                         \
    }                                                                 \
  }

enum func_type {
  PIPELINE_FUNC_PRE_PAUSE,
  PIPELINE_FUNC_SCHED,
  PIPELINE_FUNC_SETUP,
  PIPELINE_FUNC_FETCH,
  PIPELINE_FUNC_MAIN,
  PIPELINE_FUNC_THROW,
  PIPELINE_FUNC_SHUTDOWN,
  PIPELINE_FUNC_FINALIZE,
  PIPELINE_FUNC_FREEUP,
  PIPELINE_FUNC_MAINT,

  PIPELINE_FUNC_MAX,
};

#endif /* __TEST_LIB_UTIL_H__ */
