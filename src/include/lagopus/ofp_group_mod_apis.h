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

/**
 * @file        ofp_group_mod_apis.h
 * @brief       Agent/Data-Plane APIs for ofp_group_mod
 * @details     Describe APIs between Agent and Data-Plane for ofp_group_mod.
 */
#ifndef __LAGOPUS_OFP_GROUP_MOD_APIS_H__
#define __LAGOPUS_OFP_GROUP_MOD_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/bridge.h"

/* GroupMod */
/**
 * Bucket.
 */
struct bucket {
  TAILQ_ENTRY(bucket) entry;
  struct ofp_bucket ofp;
  struct ofp_bucket_counter counter;
  struct action_list action_list;
  struct action_list actions[LAGOPUS_ACTION_SET_ORDER_MAX];
};

/**
 * Bucket list.
 */
TAILQ_HEAD(bucket_list, bucket);

/**
 * Add entry to a group table for \b OFPT_GROUP_MOD(OFPGC_ADD).
 *
 *     @param[in]       dpid    Datapath id.
 *     @param[in]       group_mod       A pointer to \e group_mod structure.
 *     @param[in]       bucket_list     A pointer to list of bucket.
 *     @param[out]      error   A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval  LAGOPUS_RESULT_OK       Succeeded.
 *     @retval  LAGOPUS_RESULT_ANY_FAILURES     Failed.
 *
 *     @details The \e free() of a list element is executed
 *     by the Data-Plane side.
 */
lagopus_result_t
ofp_group_mod_add(uint64_t dpid,
                  struct ofp_group_mod *group_mod,
                  struct bucket_list *bucket_list,
                  struct ofp_error *error);

/**
 * A group table which match \e group_id is updated
 * for \b OFPT_GROUP_MOD(OFPGC_MODIFY).
 *
 *     @param[in]       dpid    Datapath id.
 *     @param[in]       group_mod       A pointer to \e group_mod structure.
 *     @param[in]       bucket_list     A pointer to list of bucket.
 *     @param[out]      error   A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval  LAGOPUS_RESULT_OK       Succeeded.
 *     @retval  LAGOPUS_RESULT_ANY_FAILURES     Failed.
 *
 *     @details The \e free() of a list element is executed
 *     by the Data-Plane side.
 */
lagopus_result_t
ofp_group_mod_modify(uint64_t dpid,
                     struct ofp_group_mod *group_mod,
                     struct bucket_list *bucket_list,
                     struct ofp_error *error);

/**
 * A group table which match \e group_id is deleted
 * for \b OFPT_GROUP_MOD(OFPGC_DELETE).
 *
 *     @param[in]       dpid    Datapath id.
 *     @param[in]       group_mod       A pointer to \e group_mod structure.
 *     @param[out]      error   A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval  LAGOPUS_RESULT_OK       Succeeded.
 *     @retval  LAGOPUS_RESULT_ANY_FAILURES     Failed.
 */
lagopus_result_t
ofp_group_mod_delete(uint64_t dpid,
                     struct ofp_group_mod *group_mod,
                     struct ofp_error *error);
/* GroupMod END */

#endif /* __LAGOPUS_OFP_GROUP_MOD_APIS_H__ */
