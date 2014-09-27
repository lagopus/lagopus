/* Generic netlink. */

#include <net/genetlink.h>
#include "genetlink.h"

struct vswitch_flow {
  int dummy;
};

#define VSWITCH_FLOW_FAMILY "vswitch flow"
#define VSWITCH_VERSION 0x01
#define VSWITCH_ATTR_MAX 10
#define VSWITCH_FLOW_CMD_NEW  1

static struct sk_buff *genl_skb = NULL;

int
vswitch_flow_set(struct sk_buff *skb, struct genl_info *info) {
  return 0;
}

static struct genl_family vswitch_flow_family = {
  .id = GENL_ID_GENERATE,
  .hdrsize = sizeof(struct vswitch_flow),
  .name = VSWITCH_FLOW_FAMILY,
  .version = VSWITCH_VERSION,
  .maxattr = VSWITCH_ATTR_MAX,
  .netnsok = true,
};

static struct genl_ops vswitch_flow_ops[] = {
  { .cmd = VSWITCH_FLOW_CMD_NEW,
    .flags = 0,                        /* Temporary all user can set. */
    /* .policy = flow_policy, */
    .doit = vswitch_flow_set
  },
};

int
genetlink_register(void) {
  int ret = 0;

  ret = genl_register_family_with_ops(&vswitch_flow_family,
                                      vswitch_flow_ops, 1);
  if (ret)
    return ret;

  genl_skb = genlmsg_new(0, GFP_KERNEL);
  if (genl_skb == NULL) {
    genetlink_unregister();
  }

  return 0;
}

int
genetlink_unregister(void) {
  int ret = 0;

  kfree_skb(genl_skb);
  ret = genl_unregister_family(&vswitch_flow_family);

  return ret;
}
