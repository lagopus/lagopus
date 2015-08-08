/* Match. */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>

#include "openflow.h"
#include "match.h"

/* Index for the match. */
struct match_index {
  ;
};

/* Preparation for the match.  */
int
prematch(struct sk_buff *skb, u16 in_port, struct match_index *index) {
  return 0;
}

/* Execute match. */
int
execute_match(struct sk_buff *skb, struct match *match) {
  return 0;
}
