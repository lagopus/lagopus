/* Action structure. */

struct action {
  uint16_t type;
  uint16_t length;
};

int execute_action(struct sk_buff *skb, struct action *action);
