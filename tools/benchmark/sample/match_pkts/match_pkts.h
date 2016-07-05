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
 * @file	match_pkts.h
 */

#ifndef __BENCHMARK_MATCH_PKTS_H__
#define __BENCHMARK_MATCH_PKTS_H__

lagopus_result_t
setup(void *pkts, size_t size);

lagopus_result_t
teardown(void *pkts, size_t size);

lagopus_result_t
match_pkts(void *pkts, size_t size);

#endif /*__BENCHMARK_MATCH_PKTS_H__ */
