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

#ifndef __LAGOPUS_HEAPCHECK_H__
#define __LAGOPUS_HEAPCHECK_H__





void
lagopus_heapcheck_module_initialize(void);

bool
lagopus_heapcheck_is_in_heap(const void *addr);

#if 0
bool
lagopus_heapcheck_is_mallocd(const void *addr);
#endif





#endif /* __LAGOPUS_HEAPCHECK_H__ */

