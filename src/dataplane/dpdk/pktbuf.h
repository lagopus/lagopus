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
 *      @file pktbuf.h
 */

#ifndef SRC_DATAPLANE_DPDK_PKTBUF_H_
#define SRC_DATAPLANE_DPDK_PKTBUF_H_

#include <rte_config.h>
#include <rte_memcpy.h>
#include <rte_mbuf.h>
#include <rte_byteorder.h>

/**
 * packet and buffer related macro definition for Intel DPDK
 */
#define OS_MBUF       struct rte_mbuf
#define OS_M_PKTLEN   rte_pktmbuf_pkt_len
#define OS_M_PREPEND  rte_pktmbuf_prepend
#define OS_M_APPEND   rte_pktmbuf_append
#define OS_M_ADJ      rte_pktmbuf_adj
#define OS_M_TRIM     rte_pktmbuf_trim
#define OS_M_FREE     rte_pktmbuf_free
#define OS_MTOD       rte_pktmbuf_mtod
#define OS_M_ADDREF(m) rte_mbuf_refcnt_update((m), 1)
#define OS_NTOHS      rte_be_to_cpu_16
#define OS_NTOHL      rte_be_to_cpu_32
#define OS_NTOHLL     rte_be_to_cpu_64
#define OS_HTONS      rte_cpu_to_be_16
#define OS_HTONL      rte_cpu_to_be_32
#define OS_HTONLL     rte_cpu_to_be_64
#define OS_MEMCPY     (void)rte_memcpy

#endif /* SRC_DATAPLANE_DPDK_PKTBUF_H_ */
