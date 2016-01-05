/* %COPYRIGHT% */

/**
 * @file	mactable_cmd.h
 */

#ifndef __LAGOPUS_DATASTORE_MACTABLE_CMD_H__
#define __LAGOPUS_DATASTORE_MACTABLE_CMD_H__

typedef struct datastore_macentry {
  uint64_t mac_addr;
  uint32_t port_no;
  uint32_t update_time;
  uint16_t address_type;
} datastore_macentry_t;

#endif /* __MACTABLE_CMD_H__ */
