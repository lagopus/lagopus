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


#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "ofp_apis.h"
#include "openflow.h"
#include "lagopus/dpmgr.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/meter.h"

/*
 * meter_mod
 */
lagopus_result_t
ofp_meter_mod_add_ctrler(uint64_t dpid,
                         struct ofp_meter_mod *meter_mod,
                         struct meter_band_list *band_list,
                         struct ofp_error *error) {
  struct dpmgr *dpmgr;
  struct bridge *bridge;
  lagopus_result_t ret;

  dpmgr = dpmgr_get_instance();
  bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  ret = meter_table_meter_add(bridge->meter_table, meter_mod,
                              band_list, error);
  /* Free is Data-Store side. */
  ofp_meter_band_list_elem_free(band_list);
  return ret;
}

lagopus_result_t
ofp_meter_mod_modify_ctrler(uint64_t dpid,
                            struct ofp_meter_mod *meter_mod,
                            struct meter_band_list *band_list,
                            struct ofp_error *error) {
  struct dpmgr *dpmgr;
  struct bridge *bridge;
  lagopus_result_t ret;

  dpmgr = dpmgr_get_instance();
  bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  ret = meter_table_meter_modify(bridge->meter_table, meter_mod,
                                 band_list, error);

  /* Free is Data-Store side. */
  ofp_meter_band_list_elem_free(band_list);
  return ret;
}

lagopus_result_t
ofp_meter_mod_delete_ctrler(uint64_t dpid,
                            struct ofp_meter_mod *meter_mod,
                            struct ofp_error *error) {
  struct dpmgr *dpmgr;
  struct bridge *bridge;

  dpmgr = dpmgr_get_instance();
  bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return meter_table_meter_delete(bridge->meter_table, meter_mod, error);
}
