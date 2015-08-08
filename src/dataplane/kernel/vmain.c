/* main routine for module. */

#include <linux/module.h>
#include "genetlink.h"

static int __init vswitch_init(void) {
  int ret = 0;

  printk(KERN_DEBUG "vswitch_init is called");
  ret = genetlink_register();

  return ret;
}

void vswitch_finish(void) {
  genetlink_unregister();
}

module_init(vswitch_init);
module_exit(vswitch_finish);

MODULE_LICENSE("GPL");
