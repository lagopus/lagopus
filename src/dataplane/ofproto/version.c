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
 *      @file   version.c
 *      @brief  version information API.
 */

#include "lagopus_config.h"

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/flowdb.h"
#include "lagopus/dataplane.h"

#ifdef HAVE_DPDK
#include <rte_config.h>
#include <rte_memory.h>
#include <rte_atomic.h>
#include <rte_version.h>
#include "dpdk/dpdk.h"
#endif /* HAVE_DPDK */

void
copy_dataplane_info(char *buf, int len) {
  snprintf(buf, (size_t)len, "Lagopus dataplane %s (%s%s) %s flowcache",
           LAGOPUS_DATAPLANE_VERSION,
#ifdef HAVE_DPDK
           "DPDK ", rte_version(),
           app.no_cache ? "without" : "with CRC32"
#else
           "raw socket", "",
           "without"
#endif /* HAVE_DPDK */
          );
}
