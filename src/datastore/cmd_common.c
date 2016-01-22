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

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "cmd_common.h"
#include "openflow13.h"

#define IS_VALID_DEL_NAME(_name) ((*(_name) == '-' || *(_name) == '~') ? \
                                  true : false)
#define IS_VALID_ADD_NAME(_name) ((*(_name) == '+') ? true : false)


lagopus_result_t
sub_cmd_add(const char *name, sub_cmd_proc_t proc,
            lagopus_hashmap_t *table) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = (void *)proc;

  if (table != NULL && *table  != NULL &&
      name != NULL && proc != NULL) {
    ret = lagopus_hashmap_add(table, (void *)name, &val, false);
  }

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

lagopus_result_t
opt_add(const char *name, opt_proc_t proc,
        lagopus_hashmap_t *table) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = (void *)proc;

  if (table != NULL && *table  != NULL &&
      name != NULL && proc != NULL) {
    ret = lagopus_hashmap_add(table, (void *)name, &val, false);
  }

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

lagopus_result_t
cmd_opt_name_get(const char *in_name,
                 char **out_name,
                 bool *is_added) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t len = 0;

  if (IS_VALID_STRING(in_name) &&
      out_name != NULL && *out_name == NULL &&
      is_added != NULL) {
    *out_name = strdup(in_name);

    if (*out_name != NULL) {
      if (IS_VALID_ADD_NAME(*out_name) ||
          IS_VALID_DEL_NAME(*out_name)) {
        if (IS_VALID_DEL_NAME(*out_name) == true) {
          *is_added = false;
        } else {
          *is_added = true;
        }
        /* delete head character. */
        len = strlen(*out_name);
        memmove(*out_name, *out_name + 1, len);
        (*out_name)[len - 1] = '\0';
      } else {
        *is_added = true;
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
cmd_opt_type_get(const char *in_type,
                 char **out_type,
                 bool *is_added) {
  return cmd_opt_name_get(in_type, out_type, is_added);
}

static bool
is_hex(char c) {
  if ((c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F')) {
    return true;
  }

  return false;
}

#define MAC_ADDR_STR_OCTET_LEN 2

lagopus_result_t
cmd_mac_addr_parse(const char *const str,  mac_address_t addr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *addrs[MAC_ADDR_STR_LEN + 1];
  char *addr_str = NULL;
  char *c = NULL;
  size_t i;

  if (IS_VALID_STRING(str) == true && addr != NULL) {
    addr_str = strdup(str);
    if (addr_str != NULL) {
      ret = lagopus_str_tokenize(addr_str, addrs,
                                 MAC_ADDR_STR_LEN + 1, ":");
      if (ret == MAC_ADDR_STR_LEN) {
        for (i = 0; i < MAC_ADDR_STR_LEN; i++) {
          if (strlen(addrs[i]) == MAC_ADDR_STR_OCTET_LEN) {
            c = addrs[i];
            while (*c != '\0') {
              if (is_hex(*c) == false) {
                ret = LAGOPUS_RESULT_OUT_OF_RANGE;
                goto done;
              }
              c++;
            }
            addr[i] = (uint8_t) strtol(addrs[i], NULL, 16);
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = LAGOPUS_RESULT_OUT_OF_RANGE;
            lagopus_perror(ret);
          }
        }
      } else {
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        lagopus_perror(ret);
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  /* free. */
  free(addr_str);

  return ret;
}

lagopus_result_t
cmd_uint_parse(const char *const str,
               enum cmd_uint_type type,
               union cmd_uint *cmd_uint) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *num_str = NULL;
  uint64_t num;

  if (IS_VALID_STRING(str) == true && type < CMD_UINT_MAX &&
      cmd_uint != NULL) {
    num_str = strdup(str);
    if (num_str != NULL) {
      if ((ret = lagopus_str_parse_uint64(num_str, &num)) ==
          LAGOPUS_RESULT_OK) {
        switch (type) {
          case CMD_UINT8:
            /* check uint8. */
            if (num <= UINT8_MAX) {
              cmd_uint->uint8 = (uint8_t) num;
              ret = LAGOPUS_RESULT_OK;
            } else {
              ret = LAGOPUS_RESULT_OUT_OF_RANGE;
              lagopus_perror(ret);
            }
            break;
          case CMD_UINT16:
            /* check uint16. */
            if (num <= UINT16_MAX) {
              cmd_uint->uint16 = (uint16_t) num;
              ret = LAGOPUS_RESULT_OK;
            } else {
              ret = LAGOPUS_RESULT_OUT_OF_RANGE;
              lagopus_perror(ret);
            }
            break;
          case CMD_UINT32:
            /* check uint32. */
            if (num <= UINT32_MAX) {
              cmd_uint->uint32 = (uint32_t) num;
              ret = LAGOPUS_RESULT_OK;
            } else {
              ret = LAGOPUS_RESULT_OUT_OF_RANGE;
              lagopus_perror(ret);
            }
            break;
          default:
            /* uint64. */
            cmd_uint->uint64 = (uint64_t) num;
            ret = LAGOPUS_RESULT_OK;
            break;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* free. */
  free(num_str);

  return ret;
}

lagopus_result_t
cmd_opt_macaddr_get(const char *in_mac, char **out_mac, bool *is_added) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t len = 0;

  if (IS_VALID_STRING(in_mac) &&
      out_mac != NULL && *out_mac == NULL &&
      is_added != NULL) {
    *out_mac = strdup(in_mac);

    if (*out_mac != NULL) {
      if (IS_VALID_ADD_NAME(*out_mac) ||
          IS_VALID_DEL_NAME(*out_mac)) {
        if (IS_VALID_DEL_NAME(*out_mac) == true) {
          *is_added = false;
        } else {
          *is_added = true;
        }
        /* delete head character. */
        len = strlen(*out_mac);
        memmove(*out_mac, *out_mac + 1, len);
        (*out_mac)[len - 1] = '\0';
      } else {
        *is_added = true;
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

