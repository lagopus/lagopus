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
#include "lagopus/datastore.h"
#include "lagopus/flowdb.h"
#include "lagopus/dp_apis.h"
#include "cmd_common.h"
#include "group_cmd.h"
#include "group_cmd_internal.h"
#include "flow_cmd.h"
#include "conv_json.h"
#include "../agent/ofp_group_handler.h"

/* command name. */
#define CMD_NAME "group"

/* stats num. */
enum g_stats {
  STATS_NAME = 0,
  STATS_GROUPS,
  STATS_GROUP_ID,
  STATS_REF_COUNT,
  STATS_PACKET_COUNT,
  STATS_BYTE_COUNT,
  STATS_DURATION_SEC,
  STATS_DURATION_NSEC,
  STATS_BUCKET,
  STATS_BUCKET_PACKET_COUNT,
  STATS_BUCKET_BYTE_COUNT,
  STATS_BUCKET_ID, /* OFP1.3 is unsupported. */

  STATS_MAX,
};

/* dump num. */
enum g_dumps {
  DUMPS_NAME = 0,
  DUMPS_GROUPS,
  DUMPS_GROUP_ID,
  DUMPS_TYPE,
  DUMPS_BUCKETS,
  DUMPS_BUCKET_WEIGHT,
  DUMPS_BUCKET_WATCH_PORT,
  DUMPS_BUCKET_WATCH_GROUP,
  DUMPS_BUCKET_ACTIONS,
  DUMPS_BUCKET_ID, /* OFP1.3 is unsupported. */

  DUMPS_MAX,
};

/* stats name. */
static const char *const stat_strs[STATS_MAX] = {
  "*name",               /* STATS_NAME (not option) */
  "*groups",             /* STATS_GROUPS (not option) */
  "*group-id",           /* STATS_GROUP_ID (not option) */
  "*ref-count",          /* STATS_REF_COUNT (not option) */
  "*packet-count",       /* STATS_PACKET_COUNT (not option) */
  "*byte-count",         /* STATS_BYTE_COUNT (not option) */
  "*duration-sec",       /* STATS_DURATION_SEC (not option) */
  "*duration-nsec",      /* STATS_DURATION_NSEC (not option) */
  "*bucket-stats",       /* STATS_BUCKET (not option) */
  "*packet-count",       /* STATS_BUCKET_PACKET_COUNT (not option) */
  "*byte-count",         /* STATS_BUCKET_BYTE_COUNT (not option) */
  "*bucket-id",          /* STATS_BUCKET_ID (not option) */
};

/* dumps name. */
static const char *const dump_strs[DUMPS_MAX] = {
  "*name",               /* DUMPS_NAME (not option) */
  "*groups",             /* DUMPS_GROUPS (not option) */
  "*group-id",           /* DUMP_GROUP_ID (not option) */
  "*type",               /* TYPE (not option) */
  "*buckets",            /* BUCKETS (not option) */
  "*weight",             /* BUCKET_WEIGHT (not option) */
  "*watch-port",         /* BUCKET_WATCH_PORT (not option) */
  "*watch-group",        /* BUCKET_WATCH_GROUP (not option) */
  "*actions",            /* BUCKET_ACTIONS (not option) */
  "*bucket-id",          /* DUMPS_BUCKET_ID (not option) */
};

/* type num. */
enum g_types {
  TYPE_ALL,             /* OFPGT_ALL */
  TYPE_SELECT,          /* OFPGT_SELECT */
  TYPE_INDIRECT,        /* OFPGT_INDIRECT */
  TYPE_FF,              /* OFPGT_FF */

  TYPE_MAX,
};

/* type name. */
static const char *const type_strs[] = {
  "all",                 /* TYPE_ALL */
  "select",              /* TYPE_SELECT */
  "indirect",            /* TYPE_INDIRECT */
  "ff",                  /* TYPE_FF */
};

typedef struct configs {
  bool is_dump;
  bool is_show_stats;
  datastore_bridge_group_info_list_t dump_list;
  datastore_bridge_group_stats_list_t stats_list;
} configs_t;

static lagopus_hashmap_t sub_cmd_table = NULL;

static inline void
group_info_list_elem_free(datastore_bridge_group_info_list_t *list) {
  struct bucket *bucket = NULL;
  struct group_desc *info = NULL;

  while ((info = TAILQ_FIRST(list)) != NULL) {
    while ((bucket = TAILQ_FIRST(&info->bucket_list)) != NULL) {
      action_list_entry_free(&bucket->action_list);
      TAILQ_REMOVE(&info->bucket_list, bucket, entry);
      free(bucket);
    }
    TAILQ_REMOVE(list, info, entry);
    free(info);
  }
}

static lagopus_result_t
stats_sub_cmd_parse(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    size_t argc, const char *const argv[],
                    char *name,
                    lagopus_hashmap_t *hptr,
                    datastore_update_proc_t proc,
                    void *out_configs,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  configs_t *configs = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;

    if (bridge_exists(name) == true) {
      if (*(argv + 1) == NULL) {
        configs->is_show_stats = true;
        ret = dp_bridge_group_stats_list_get(name, &configs->stats_list);

        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get stats.");
        }
      } else {
        ret = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_INVALID_ARGS,
            "Bad opt = %s.", *(argv + 1));
        goto done;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_OBJECT,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_parse(const char *name,
           configs_t *out_configs,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  configs_t *configs = NULL;

  if (name != NULL && out_configs != NULL &&
      result != NULL) {
    configs = (configs_t *) out_configs;

    if (bridge_exists(name) == true) {
      configs->is_dump = true;
      ret = dp_bridge_group_list_get(name, &configs->dump_list);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get info list.");
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_OBJECT,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline const char *
dump_type(uint8_t type) {
  switch(type) {
    case OFPGT_ALL:
      return type_strs[TYPE_ALL];
      break;
    case OFPGT_SELECT:
      return type_strs[TYPE_SELECT];
      break;
    case OFPGT_INDIRECT:
      return type_strs[TYPE_INDIRECT];
      break;
    case OFPGT_FF:
      return type_strs[TYPE_FF];
      break;
    default:
      return "unknown";
      break;
  }
}

static lagopus_result_t
group_cmd_dump_json_create(char *name,
                           lagopus_dstring_t *ds,
                           configs_t *configs,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bucket *bucket = NULL;
  struct action *action = NULL;
  struct group_desc *info = NULL;
  uint32_t bucket_id = 0;
  bool delimiter1;
  bool delimiter2;
  bool is_action_first = true;

  ret = lagopus_dstring_appendf(ds, "[");
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_appendf(ds, "{");
    if (ret == LAGOPUS_RESULT_OK) {
      /* name */
      if ((ret = datastore_json_string_append(
              ds, ATTR_NAME_GET(dump_strs, DUMPS_NAME),
              name, false)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      /* groups */
      if ((ret = DSTRING_CHECK_APPENDF(
              ds, true,
              KEY_FMT"[",
              ATTR_NAME_GET(dump_strs, DUMPS_GROUPS))) !=
         LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if (TAILQ_EMPTY(&configs->dump_list) == false) {
        delimiter1 = false;
        TAILQ_FOREACH(info, &configs->dump_list, entry) {
          if ((ret = DSTRING_CHECK_APPENDF(
                  ds, delimiter1, "{")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* group_id */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(dump_strs, DUMPS_GROUP_ID),
                  info->ofp.group_id, false)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* type */
          if ((ret = datastore_json_string_append(
                  ds, ATTR_NAME_GET(dump_strs, DUMPS_TYPE),
                  dump_type(info->ofp.type), true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* buckets */
          if ((ret = DSTRING_CHECK_APPENDF(
                  ds, true,
                  KEY_FMT"[",
                  ATTR_NAME_GET(dump_strs, DUMPS_BUCKETS))) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if (TAILQ_EMPTY(&info->bucket_list) == false) {
            delimiter2 = false;
            TAILQ_FOREACH(bucket, &info->bucket_list, entry) {
              if ((ret = DSTRING_CHECK_APPENDF(
                      ds, delimiter2, "{")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* bucket_id */
              if ((ret = datastore_json_uint32_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BUCKET_ID),
                      bucket_id, false)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* weight */
              if ((ret = datastore_json_uint16_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BUCKET_WEIGHT),
                      bucket->ofp.weight, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* watch_port */
              if ((ret = datastore_json_uint32_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BUCKET_WATCH_PORT),
                      bucket->ofp.watch_port, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* watch_group */
              if ((ret = datastore_json_uint32_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BUCKET_WATCH_GROUP),
                      bucket->ofp.watch_group, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* actions */
              if ((ret = DSTRING_CHECK_APPENDF(
                      ds, true,
                      KEY_FMT"[",
                      ATTR_NAME_GET(dump_strs, DUMPS_BUCKET_ACTIONS))) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
              if (TAILQ_EMPTY(&bucket->action_list) == false) {
                is_action_first = true;
                TAILQ_FOREACH(action, &bucket->action_list, entry) {
                  if ((ret = flow_cmd_dump_action(action,
                                                  is_action_first,
                                                  ds)) !=
                      LAGOPUS_RESULT_OK) {
                    lagopus_perror(ret);
                    goto done;
                  }
                  if (is_action_first == true) {
                    is_action_first = false;
                  }
                }
              }

              if ((ret = lagopus_dstring_appendf(ds, "]")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              if ((ret = lagopus_dstring_appendf(ds, "}")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
              if (delimiter2 == false) {
                delimiter2 = true;
              }

              bucket_id++;
            }
          }

          if ((ret = lagopus_dstring_appendf(ds, "]")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if ((ret = lagopus_dstring_appendf(ds, "}")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if (delimiter1 == false) {
            delimiter1 = true;
          }
        }
      } else {
        /* list is empty. */
        ret = LAGOPUS_RESULT_OK;
      }

      if (ret == LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(ds, "]")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if ((ret = lagopus_dstring_appendf(ds, "}")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_appendf(ds, "]");
        }
      }
    }
  }

done:
  if (ret != LAGOPUS_RESULT_OK &&
      ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  return ret;
}

static lagopus_result_t
group_cmd_stats_json_create(char *name,
                            lagopus_dstring_t *ds,
                            configs_t *configs,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct group_stats *stats = NULL;
  struct bucket_counter *bucket_counter = NULL;
  uint32_t bucket_id = 0;
  bool delimiter1;
  bool delimiter2;

  ret = lagopus_dstring_appendf(ds, "[");
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_appendf(ds, "{");
    if (ret == LAGOPUS_RESULT_OK) {
      /* name */
      if ((ret = datastore_json_string_append(
              ds, ATTR_NAME_GET(stat_strs, STATS_NAME),
              name, false)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      /* groups */
      if ((ret = DSTRING_CHECK_APPENDF(
              ds, true,
              KEY_FMT"[",
              ATTR_NAME_GET(stat_strs, STATS_GROUPS))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if (TAILQ_EMPTY(&configs->stats_list) == false) {
        delimiter1 = false;
        TAILQ_FOREACH(stats, &configs->stats_list, entry) {
          if ((ret = DSTRING_CHECK_APPENDF(
                  ds, delimiter1, "{")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* group_id */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_GROUP_ID),
                  stats->ofp.group_id, false)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* ref_count */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_REF_COUNT),
                  stats->ofp.ref_count, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* packet_count */
          if ((ret = datastore_json_uint64_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_PACKET_COUNT),
                  stats->ofp.packet_count, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* byte_count */
          if ((ret = datastore_json_uint64_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_BYTE_COUNT),
                  stats->ofp.byte_count, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* duration_sec */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_DURATION_SEC),
                  stats->ofp.duration_sec, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* duration_nsec */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_DURATION_NSEC),
                  stats->ofp.duration_nsec, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* bucket_stats */
          if ((ret = DSTRING_CHECK_APPENDF(
                  ds, true,
                  KEY_FMT"[",
                  ATTR_NAME_GET(stat_strs, STATS_BUCKET))) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if (TAILQ_EMPTY(&stats->bucket_counter_list) == false) {
            delimiter2 = false;
            TAILQ_FOREACH(bucket_counter, &stats->bucket_counter_list, entry) {
              if ((ret = DSTRING_CHECK_APPENDF(
                      ds, delimiter2, "{")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* bucket_id */
              if ((ret = datastore_json_uint32_append(
                      ds, ATTR_NAME_GET(stat_strs, STATS_BUCKET_ID),
                      bucket_id, false)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* packet_count */
              if ((ret = datastore_json_uint64_append(
                      ds, ATTR_NAME_GET(stat_strs, STATS_BUCKET_PACKET_COUNT),
                      bucket_counter->ofp.packet_count, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* byte_count */
              if ((ret = datastore_json_uint64_append(
                      ds, ATTR_NAME_GET(stat_strs, STATS_BUCKET_BYTE_COUNT),
                      bucket_counter->ofp.byte_count, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              if ((ret = lagopus_dstring_appendf(ds, "}")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              if (delimiter2 == false) {
                delimiter2 = true;
              }

              bucket_id++;
            }
          }

          if ((ret = lagopus_dstring_appendf(ds, "]")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if ((ret = lagopus_dstring_appendf(ds, "}")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if (delimiter1 == false) {
            delimiter1 = true;
          }
        }
      } else {
        /* list is empty. */
        ret = LAGOPUS_RESULT_OK;
      }

      if (ret == LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(ds, "]")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if ((ret = lagopus_dstring_appendf(ds, "}")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_appendf(ds, "]");
        }
      }
    }
  }

done:
  if (ret != LAGOPUS_RESULT_OK &&
      ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  return ret;
}

STATIC lagopus_result_t
group_cmd_parse(datastore_interp_t *iptr,
                datastore_interp_state_t state,
                size_t argc, const char *const argv[],
                lagopus_hashmap_t *hptr,
                datastore_update_proc_t u_proc,
                datastore_enable_proc_t e_proc,
                datastore_serialize_proc_t s_proc,
                datastore_destroy_proc_t d_proc,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t ret_for_json = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  void *sub_cmd_proc;
  configs_t out_configs = {false, false, {0LL}, {0LL}};
  char *name = NULL;
  char *fullname = NULL;
  char *str = NULL;
  lagopus_dstring_t conf_result = NULL;

  (void)e_proc;
  (void)s_proc;
  (void)d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  if (iptr != NULL && argv != NULL && result != NULL) {
    TAILQ_INIT(&out_configs.dump_list);
    TAILQ_INIT(&out_configs.stats_list);
    if ((ret = lagopus_dstring_create(&conf_result)) == LAGOPUS_RESULT_OK) {
      argv++;

      if (IS_VALID_STRING(*argv) == true) {
        if ((ret = lagopus_str_unescape(*argv, "\"'", &name)) < 0) {
          goto done;
        } else {
          argv++;

          if ((ret = namespace_get_fullname(name, &fullname))
              == LAGOPUS_RESULT_OK) {
            if (IS_VALID_STRING(*argv) == true) {
              if ((ret = lagopus_hashmap_find(
                           &sub_cmd_table,
                           (void *)(*argv),
                           &sub_cmd_proc)) == LAGOPUS_RESULT_OK) {
                /* parse sub cmd. */
                if (sub_cmd_proc != NULL) {
                  ret_for_json =
                    ((sub_cmd_proc_t) sub_cmd_proc)(iptr, state,
                                                    argc, argv,
                                                    fullname, hptr,
                                                    u_proc,
                                                    (void *) &out_configs,
                                                    result);
                } else {
                  ret = LAGOPUS_RESULT_NOT_FOUND;
                  lagopus_perror(ret);
                  goto done;
                }
              } else {
                ret_for_json = datastore_json_result_string_setf(
                                 result, LAGOPUS_RESULT_INVALID_ARGS,
                                 "sub_cmd = %s.", *argv);
              }
            } else {
              ret_for_json = dump_parse(fullname, &out_configs,
                                        result);
            }
          } else {
            ret_for_json = datastore_json_result_string_setf(
                result, ret, "Can't get fullname %s.", name);
          }
        }
      } else {
        ret_for_json = datastore_json_result_set(result,
                                                 LAGOPUS_RESULT_INVALID_ARGS,
                                                 NULL);
      }

      /* create json for conf. */
      if (ret_for_json == LAGOPUS_RESULT_OK) {
        if (out_configs.is_show_stats == true) {
          ret = group_cmd_stats_json_create(fullname, &conf_result, &out_configs,
                                            result);
        } else if (out_configs.is_dump){
          ret =  group_cmd_dump_json_create(fullname, &conf_result, &out_configs,
                                            result);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_str_get(&conf_result, &str);
          if (ret != LAGOPUS_RESULT_OK) {
            goto done;
          }
        } else {
          goto done;
        }
        ret = datastore_json_result_set(result, LAGOPUS_RESULT_OK, str);
      } else {
        ret = ret_for_json;
      }
    }

  done:
    /* free. */
    free(name);
    free(fullname);
    group_stats_list_elem_free(&out_configs.stats_list);
    group_info_list_elem_free(&out_configs.dump_list);
    free(str);
    lagopus_dstring_destroy(&conf_result);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

extern datastore_interp_t datastore_get_master_interp(void);

static inline lagopus_result_t
initialize_internal(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_t s_interp = datastore_get_master_interp();

  /* create hashmap for sub cmds. */
  if ((ret = lagopus_hashmap_create(&sub_cmd_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (((ret = sub_cmd_add(STATS_SUB_CMD, stats_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, group_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

done:
  return ret;
}

/*
 * Public:
 */

lagopus_result_t
group_cmd_initialize(void) {
  return initialize_internal();
}

void
group_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
}
