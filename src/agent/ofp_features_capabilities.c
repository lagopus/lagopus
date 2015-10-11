/* %COPYRIGHT% */

#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "lagopus/datastore.h"
#include "ofp_features_capabilities.h"

typedef struct capability_proc_info {
  uint32_t features_capability;
  uint64_t ds_capability;
} capability_proc_info_t;

static const capability_proc_info_t capabilities[] = {
  {OFPC_FLOW_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_FLOW_STATS},
  {OFPC_TABLE_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_TABLE_STATS},
  {OFPC_PORT_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_PORT_STATS},
  {OFPC_GROUP_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_GROUP_STATS},
  {OFPC_IP_REASM, DATASTORE_BRIDGE_BIT_CAPABILITY_REASSEMBLE_IP_FRGS},
  {OFPC_QUEUE_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_QUEUE_STATS},
  {OFPC_PORT_BLOCKED, DATASTORE_BRIDGE_BIT_CAPABILITY_BLOCK_LOOPING_PORTS}
};

static const size_t capabilities_size =
    sizeof(capabilities) / sizeof(capability_proc_info_t);

static void
features_reply_features_capability_set(uint64_t ds_capability,
                                       size_t i,
                                       uint32_t *flags) {
  if ((ds_capability & capabilities[i].ds_capability) == 0) {
    *flags &= ~capabilities[i].features_capability;
  } else {
    *flags |= capabilities[i].features_capability;
  }
}

lagopus_result_t
ofp_features_capabilities_convert(uint64_t ds_capabilities,
                                  uint32_t *features_capabilities) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  if (features_capabilities != NULL) {
    /* set capabilities. */
    *features_capabilities = 0;
    for (i = 0; i < capabilities_size; i++) {
      features_reply_features_capability_set(
          ds_capabilities, i,
          features_capabilities);
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
