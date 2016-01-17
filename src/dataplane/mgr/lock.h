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

#ifndef SRC_DATAPLANE_MGR_LOCK_H_
#define SRC_DATAPLANE_MGR_LOCK_H_

#include "lagopus_config.h"

#ifdef HAVE_DPDK
#include "rte_config.h"
#include "rte_rwlock.h"
#endif /* HAVE_DPDK */

/*
 * flowdb lock primitive.
 */
#ifdef HAVE_DPDK
rte_rwlock_t flowdb_update_lock;
rte_rwlock_t dpmgr_lock;

#define FLOWDB_RWLOCK_INIT()
#define FLOWDB_RWLOCK_RDLOCK()  do {                                    \
    rte_rwlock_read_lock(&dpmgr_lock);                                  \
  } while(0)
#define FLOWDB_RWLOCK_WRLOCK()  do {                                    \
    rte_rwlock_write_lock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_RDUNLOCK() do {                                   \
    rte_rwlock_read_unlock(&dpmgr_lock);                                \
  } while(0)
#define FLOWDB_RWLOCK_WRUNLOCK() do {                                   \
    rte_rwlock_write_unlock(&dpmgr_lock);                               \
  } while(0)
#define FLOWDB_UPDATE_CHECK() do {                      \
    rte_rwlock_read_lock(&flowdb_update_lock);          \
    rte_rwlock_read_unlock(&flowdb_update_lock);        \
  } while (0)
#define FLOWDB_UPDATE_BEGIN() do {               \
    rte_rwlock_write_lock(&flowdb_update_lock);  \
  } while (0)
#define FLOWDB_UPDATE_END() do {                        \
    rte_rwlock_write_unlock(&flowdb_update_lock);       \
  } while (0)
#else
pthread_rwlock_t flowdb_update_lock;
pthread_rwlock_t dpmgr_lock;
#define FLOWDB_RWLOCK_INIT() do {                                       \
    pthread_rwlock_init(&flowdb_update_lock, NULL);                     \
    pthread_rwlock_init(&dpmgr_lock, NULL);                             \
  } while(0)
#define FLOWDB_RWLOCK_RDLOCK()  do {                                    \
    pthread_rwlock_rdlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_WRLOCK()  do {                                    \
    pthread_rwlock_wrlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_RDUNLOCK() do {                                   \
    pthread_rwlock_unlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_RWLOCK_WRUNLOCK() do {                                   \
    pthread_rwlock_unlock(&dpmgr_lock);                                 \
  } while(0)
#define FLOWDB_UPDATE_CHECK() do {                      \
    pthread_rwlock_rdlock(&flowdb_update_lock);    \
    pthread_rwlock_unlock(&flowdb_update_lock);     \
  } while (0)
#define FLOWDB_UPDATE_BEGIN() do {                      \
    pthread_rwlock_wrlock(&flowdb_update_lock);    \
  } while (0)
#define FLOWDB_UPDATE_END() do {                        \
    pthread_rwlock_unlock(&flowdb_update_lock);     \
  } while (0)
#endif /* HAVE_DPDK */

/**
 * Initialize lock of the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
void flowdb_lock_init(struct flowdb *flowdb);

/**
 * Read lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
static inline void
flowdb_rdlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_RDLOCK();
}

/**
 * Check write lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
static inline void
flowdb_check_update(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_UPDATE_CHECK();
}

/**
 * Write lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be locked.
 */
static inline void
flowdb_wrlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_UPDATE_BEGIN();
  FLOWDB_RWLOCK_WRLOCK();
}

/**
 * Unlock read lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be unlocked.
 */
static inline void
flowdb_rdunlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_RDUNLOCK();
}

/**
 * Unlock write lock the flow database.
 *
 * @param[in]   flowdb  Flow database to be unlocked.
 */
static inline void
flowdb_wrunlock(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_WRUNLOCK();
  FLOWDB_UPDATE_END();
}

#endif /* SRC_DATAPLANE_MGR_LOCK_H_ */
