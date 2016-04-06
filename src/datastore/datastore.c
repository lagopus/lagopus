/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "flow_cmd.h"
#ifdef HYBRID
#include "mactable_cmd.h"
#include "route_cmd.h"
#endif /* HYBRID */
#include "channel_cmd.h"
#include "controller_cmd.h"
#include "interface_cmd.h"
#include "port_cmd.h"
#include "bridge_cmd.h"
#include "namespace_cmd.h"
#include "meter_cmd.h"
#include "group_cmd.h"
#include "affinition_cmd.h"
#include "queue_cmd.h"
#include "policer_cmd.h"
#include "policer_action_cmd.h"
#include "ns_util.h"

lagopus_result_t
copy_mac_address(const mac_address_t src, mac_address_t dst) {
  (void)memcpy(dst, src, sizeof(mac_address_t));
  return LAGOPUS_RESULT_OK;
}

bool
equals_mac_address(const mac_address_t mac1, const mac_address_t mac2) {
  if (memcmp(mac1, mac2, sizeof(mac_address_t)) == 0) {
    return true;
  }
  return false;
}

lagopus_result_t
datastore_names_destroy(datastore_name_info_t *names) {
  struct datastore_name_entry *entry = NULL;

  if (names == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  while ((entry = TAILQ_FIRST(&(names->head))) != NULL) {
    TAILQ_REMOVE(&(names->head), entry, name_entries);
    free(entry->str);
    free(entry);
  }
  names->size = 0;

  free((void *) names);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
datastore_names_create(datastore_name_info_t **names) {
  if (names == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*names != NULL) {
    datastore_names_destroy(*names);
    *names = NULL;
  }

  if ((*names = (datastore_name_info_t *) malloc(sizeof(datastore_name_info_t)))
      == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  (*names)->size = 0;
  TAILQ_INIT(&((*names)->head));

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
datastore_names_duplicate(const datastore_name_info_t *src_names,
                          datastore_name_info_t **dst_names,
                          const char *namespace) {
  struct datastore_name_entry *src_entry = NULL;
  struct datastore_name_entry *dst_entry = NULL;

  if (src_names == NULL || dst_names == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_names != NULL) {
    datastore_names_destroy(*dst_names);
    *dst_names = NULL;
  }

  *dst_names = (datastore_name_info_t *) malloc(sizeof(datastore_name_info_t));
  if (*dst_names == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  memset(*dst_names, 0, sizeof(datastore_name_info_t));

  TAILQ_INIT(&((*dst_names)->head));

  TAILQ_FOREACH(src_entry, &(src_names->head), name_entries) {
    dst_entry = (struct datastore_name_entry *) malloc(sizeof(
        struct datastore_name_entry));
    if (dst_entry == NULL) {
      goto error;
    }
    TAILQ_INSERT_TAIL(&((*dst_names)->head), dst_entry, name_entries);
    if (namespace == NULL) {
      dst_entry->str = strdup(src_entry->str);
    } else {
      if (ns_replace_namespace(src_entry->str,
                               namespace,
                               &(dst_entry->str)) != LAGOPUS_RESULT_OK) {
        goto error;
      }
    }

    if (IS_VALID_STRING(dst_entry->str) == true) {
      (*dst_names)->size++;
    } else {
      goto error;
    }
  }

  return LAGOPUS_RESULT_OK;

error:
  datastore_names_destroy(*dst_names);
  *dst_names = NULL;
  return LAGOPUS_RESULT_NO_MEMORY;
}

bool
datastore_name_exists(const datastore_name_info_t *names,
                      const char *name) {
  struct datastore_name_entry *entry = NULL;

  if (names != NULL && name != NULL) {
    TAILQ_FOREACH(entry, &(names->head), name_entries) {
      if (strcmp(entry->str, name) == 0) {
        return true;
      }
    }
  }

  return false;
}

bool
datastore_names_equals(const datastore_name_info_t *names0,
                       const datastore_name_info_t *names1) {
  struct datastore_name_entry *entry = NULL;

  if (names0 == NULL || names1 == NULL) {
    return false;
  }

  if (names0->size != names1->size) {
    return false;
  }

  TAILQ_FOREACH(entry, &(names0->head), name_entries) {
    if (datastore_name_exists(names1, entry->str) == false) {
      return false;
    }
  }

  return true;
}

lagopus_result_t
datastore_add_names(datastore_name_info_t *names, const char *name) {
  struct datastore_name_entry *e = NULL;

  if (names == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  e = malloc(sizeof(struct datastore_name_entry));
  if (e == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  e->str = strdup(name);
  if (IS_VALID_STRING(e->str) == false) {
    free(e);
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  TAILQ_INSERT_TAIL(&(names->head), e, name_entries);
  names->size++;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
datastore_remove_names(datastore_name_info_t *names, const char *name) {
  struct datastore_name_entry *entry = NULL;

  if (names == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  TAILQ_FOREACH(entry, &(names->head), name_entries) {
    if (strcmp(entry->str, name) == 0) {
      TAILQ_REMOVE(&(names->head), entry, name_entries);
      free(entry->str);
      free(entry);

      names->size--;

      break;
    }
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
datastore_remove_all_names(datastore_name_info_t *names) {
  struct datastore_name_entry *entry = NULL;

  if (names == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  while ((entry = TAILQ_FIRST(&(names->head))) != NULL) {
    TAILQ_REMOVE(&(names->head), entry, name_entries);
    free(entry->str);
    free(entry);
  }
  names->size = 0;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
datastore_all_commands_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = interface_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = port_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = channel_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = controller_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = bridge_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = flow_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
#ifdef HYBRID
  if ((ret = mactable_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = route_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
#endif /* HYBRID */
  if ((ret = namespace_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = meter_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = group_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = affinition_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = queue_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = policer_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = policer_action_cmd_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  return ret;
}

void
datastore_all_commands_finalize(void) {
  flow_cmd_finalize();
#ifdef HYBRID
  mactable_cmd_finalize();
  route_cmd_finalize();
#endif /* HYBRID */
  interface_cmd_finalize();
  port_cmd_finalize();
  channel_cmd_finalize();
  controller_cmd_finalize();
  bridge_cmd_finalize();
  namespace_cmd_finalize();
  meter_cmd_finalize();
  group_cmd_finalize();
  affinition_cmd_finalize();
  queue_cmd_finalize();
  policer_cmd_finalize();
  policer_action_cmd_finalize();
}
