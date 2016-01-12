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

#ifndef __QMUXER_INTERNAL_H__
#define __QMUXER_INTERNAL_H__





#define NEED_WAIT_READABLE(type) \
  (IS_BIT_SET(((int)(type)), ((int)LAGOPUS_QMUXER_POLL_READABLE)))

#define NEED_WAIT_WRITABLE(type) \
  (IS_BIT_SET(((int)(type)), ((int)LAGOPUS_QMUXER_POLL_WRITABLE)))

#define IS_VALID_POLL_TYPE(t)                                   \
  (((int)t > (int)LAGOPUS_QMUXER_POLL_UNKNOWN &&                \
    (int)t <= (int)LAGOPUS_QMUXER_POLL_BOTH) ? true : false)





void
qmuxer_notify(lagopus_qmuxer_t qmx);


lagopus_result_t
cbuffer_setup_for_qmuxer(lagopus_cbuffer_t cb,
                         lagopus_qmuxer_t qmx,
                         ssize_t *szptr,
                         ssize_t *remptr,
                         lagopus_qmuxer_poll_event_t type,
                         bool is_pre);





#endif /* ! __QMUXER_INTERNAL_H__ */
