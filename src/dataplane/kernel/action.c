/* Action. */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>

#include "openflow.h"
#include "action.h"

/* Push MPLS. */
static int
push_mpls(struct sk_buff *skb, struct ofp_action_push *push) {
  if (push->ethertype) {
    return -1;
  }

  return 0;
}

/* Pop MPLS. */
static int
pop_mpls(struct sk_buff *skb, struct ofp_action_pop_mpls *pop) {
  return 0;
}

/* Execute action. */
int
execute_action(struct sk_buff *skb, struct action *action) {
  int result;

  switch (action->type) {
    case OFPAT_OUTPUT:
      break;
    case OFPAT_COPY_TTL_OUT:
      break;
    case OFPAT_COPY_TTL_IN:
      break;
    case OFPAT_SET_MPLS_TTL:
      break;
    case OFPAT_DEC_MPLS_TTL:
      break;
    case OFPAT_PUSH_VLAN:
      break;
    case OFPAT_POP_VLAN:
      break;
    case OFPAT_PUSH_MPLS:
      result = push_mpls(skb, (struct ofp_action_push *)action);
      break;
    case OFPAT_POP_MPLS:
      result = pop_mpls(skb, (struct ofp_action_pop_mpls *)action);
      break;
    case OFPAT_SET_QUEUE:
      break;
    case OFPAT_GROUP:
      break;
    case OFPAT_SET_NW_TTL:
      break;
    case OFPAT_DEC_NW_TTL:
      break;
    case OFPAT_SET_FIELD:
      break;
    case OFPAT_PUSH_PBB:
      break;
    case OFPAT_POP_PBB:
      break;
    case OFPAT_EXPERIMENTER:
      break;
    default:
      break;
  }

  /* Return code check. */
  if (result < 0) {
    /* Error handling. */
  }

  return 0;
}
