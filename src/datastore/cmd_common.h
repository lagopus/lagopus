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

/**
 * @file	cmd_common.h
 */

#ifndef __CMD_COMMON_H__
#define __CMD_COMMON_H__

#include "conv_json.h"
#include "conv_json_result.h"
#include "datastore_internal.h"
#include "lagopus/port.h"

#define CONFIGURATOR_NAME	"lagopus.dsl"

#ifdef __UNIT_TESTING__
#define STATIC
#ifdef LAGOPUS_PORT_TYPE_PHYSICAL
#undef LAGOPUS_PORT_TYPE_PHYSICAL
#endif /* LAGOPUS_PORT_TYPE_PHYSICAL */
#define LAGOPUS_PORT_TYPE_PHYSICAL LAGOPUS_PORT_TYPE_NULL
#else
#define STATIC static
#endif /* __UNIT_TESTING__ */

#define MAC_ADDR_STR_LEN OFP_ETH_ALEN

#define CREATE_SUB_CMD "create"
#define CONFIG_SUB_CMD "config"
#define ENABLE_SUB_CMD "enable"
#define DISABLE_SUB_CMD "disable"
#define DESTROY_SUB_CMD "destroy"
#define STATS_SUB_CMD "stats"
#define DUMP_SUB_CMD "dump"
#define CLEAR_SUB_CMD "clear"
#define CLEAR_QUEUE_SUB_CMD "clear-queue"
#define ADD_SUB_CMD "add"
#define DEL_SUB_CMD "del"
#define MOD_SUB_CMD "mod"
#define SHOW_OPT_CURRENT "current"
#define SHOW_OPT_MODIFIED "modified"

/* number of update for auto commit. */
#define UPDATE_CNT_MAX 2

#define OPTS_MAX UINT64_MAX

#define IS_VALID_OPT(_name) ((*(_name) == '-') ? true : false)
#define ATTR_NAME_GET(_opts, _opt) ((_opts)[(_opt)] + 1) /* 1 is size of "-". */
#define ATTR_NAME_GET_FOR_STR(_opt) ((_opt) + 1) /* 1 is size of "-". */
#define OPT_CMP(_s, _opts, _opt) (strcmp((_s), (_opts)[(_opt)]))
#define OPT_BIT_GET(_opt) ((uint64_t) 1 << (_opt))
#define OPT_REQUIRED_CHECK(_ops, _reops)        \
  (((_ops) & (_reops)) == (_reops))
#define ESCAPE_NAME_FMT(_is_esc, _s) \
  (((_is_esc) == true || strchr(_s, ' ') != NULL) ? " \"%s\"" : " %s")

#ifdef LAGOPUS_BIG_ENDIAN
#ifndef hton24
#define hton24(_x) (_x)
#endif /* hton24 */
#ifndef ntoh24
#define ntoh24(_x) (_x)
#endif /* ntoh24 */
#else
#ifndef hton24
#define hton24(_x)                                      \
  ((uint32_t) (((_x) & 0x000000ffLL) << 16) |           \
   htons((uint16_t) (((_x) & 0x00ffff00LL) >> 8)))
#endif /* hton24 */
#ifndef ntoh24
#define ntoh24(_x)                                      \
  ((uint32_t) (((_x) & 0x000000ffLL) << 16) |           \
   ntohs((uint16_t) (((_x) & 0x00ffff00LL) >> 8)))
#endif /* ntoh24 */
#endif /* LAGOPUS_BIG_ENDIAN */

/* uint types. */
enum cmd_uint_type {
  CMD_UINT8 = 0,
  CMD_UINT16,
  CMD_UINT32,
  CMD_UINT64,

  CMD_UINT_MAX,
};

union cmd_uint {
  uint8_t uint8;
  uint16_t uint16;
  uint32_t uint32;
  uint64_t uint64;
};

typedef lagopus_result_t
(*sub_cmd_proc_t)(datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  size_t argc,
                  const char *const argv[],
                  char *name,
                  lagopus_hashmap_t *hptr,
                  datastore_update_proc_t proc,
                  void *out_configs,
                  lagopus_dstring_t *result);

typedef lagopus_result_t
(*opt_proc_t)(const char *const *argv[],
              void *c, void *out_cs,
              lagopus_dstring_t *result);

/**
 * Add sub command in hashtable.
 *
 *     @param[in]	name 	Sub command name.
 *     @param[in]	proc	An sub command parser proc.
 *     @param[in]	table	A pointer to a hashmap.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
sub_cmd_add(const char *name, sub_cmd_proc_t proc,
            lagopus_hashmap_t *table);

/**
 * Add opt in hashtable.
 *
 *     @param[in]	name 	Opt name.
 *     @param[in]	proc	An opt parser proc.
 *     @param[in]	table	A pointer to a hashmap.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
opt_add(const char *name, opt_proc_t proc,
        lagopus_hashmap_t *table);

/**
 * Get opt value name. (Delete "-", "~", "+" from head.)
 *
 *     @param[in]	in_name 	A pointer to input name.
 *     @param[out]	out_name 	A pointer to output name.
 *     @param[in]	is_added 	Add or delete flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
cmd_opt_name_get(const char *in_name,
                 char **out_name,
                 bool *is_added);

/**
 * Get opt value type. (Delete "-", "~", "+" from head.)
 *
 *     @param[in]	in_type 	A pointer to input type.
 *     @param[out]	out_type 	A pointer to output type.
 *     @param[in]	is_added 	Add or delete flag.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
cmd_opt_type_get(const char *in_type,
                 char **out_type,
                 bool *is_added);

/**
 * Parse mac addr.
 *
 *     @param[in]	str 	A pointer to input string.
 *     @param[out]	addr 	A mac_address_t structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
cmd_mac_addr_parse(const char *const str,  mac_address_t addr);

/**
 * Parse uint.
 *
 *     @param[in]	str 	A pointer to input string.
 *     @param[in]	type 	Type of uint.
 *     @param[out]	cmd_uint 	A cmd_uint union.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
lagopus_result_t
cmd_uint_parse(const char *const str,
               enum cmd_uint_type type,
               union cmd_uint *cmd_uint);

/**
 * Check if delete command.
 */
lagopus_result_t
cmd_opt_macaddr_get(const char *in_mac, char **out_mac, bool *is_added);

#endif /* __CMD_COMMON_H__ */
