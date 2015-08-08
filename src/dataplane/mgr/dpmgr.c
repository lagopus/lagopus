/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
 *      @file   dpmgr.c
 *      @brief  Datapath manager.
 */

#include "lagopus_apis.h"
#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/bridge.h"
#include "lagopus/dp_apis.h"

lagopus_result_t
dpmgr_switch_mode_get(uint64_t dpid, enum switch_mode *switch_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = flowdb_switch_mode_get(bridge->flowdb, switch_mode);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
dpmgr_switch_mode_set(uint64_t dpid, enum switch_mode switch_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = flowdb_switch_mode_set(bridge->flowdb, switch_mode);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
dpmgr_bridge_fail_mode_get(uint64_t dpid,
                           enum fail_mode *fail_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_fail_mode_get(bridge, fail_mode);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_get(uint64_t dpid, uint8_t *version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_get(bridge, version);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_set(uint64_t dpid, uint8_t version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_set(bridge, version);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_bitmap_get(uint64_t dpid,
                                    uint32_t *version_bitmap) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_bitmap_get(bridge, version_bitmap);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_bitmap_set(uint64_t dpid, uint8_t version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_bitmap_set(bridge, version);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_features_get(uint64_t dpid,
                              struct ofp_switch_features *features) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_features_get(bridge, features);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}
