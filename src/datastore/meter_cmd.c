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
#include "lagopus/dp_apis.h"
#include "cmd_common.h"
#include "meter_cmd.h"
#include "meter_cmd_internal.h"
#include "conv_json.h"
#include "../agent/ofp_meter_handler.h"

/* command name. */
#define CMD_NAME "meter"

/* stats num. */
enum m_stats {
  STATS_NAME = 0,
  STATS_METERS,
  STATS_METER_ID,
  STATS_FLOW_COUNT,
  STATS_PACKET_IN_COUNT,
  STATS_BYTE_IN_COUNT,
  STATS_DURATION_SEC,
  STATS_DURATION_NSEC,
  STATS_BAND,
  STATS_BAND_PACKET_BAND_COUNT,
  STATS_BAND_BYTE_BAND_COUNT,
  STATS_BAND_ID, /* OFP1.3 is unsupported */

  STATS_MAX,
};

/* dump num. */
enum m_dumps {
  DUMPS_NAME = 0,
  DUMPS_METERS,
  DUMPS_METER_ID,
  DUMPS_FLAGS,
  DUMPS_BANDS,
  DUMPS_BAND_TYPE,
  DUMPS_BAND_RATE,
  DUMPS_BAND_BURST_SIZE,
  DUMPS_BAND_PREC_LEVEL,
  DUMPS_BAND_EXPERIMENTER,
  DUMPS_BAND_ID, /* OFP1.3 is unsupported */

  DUMPS_MAX,
};

/* stats name. */
static const char *const stat_strs[STATS_MAX] = {
  "*name",               /* STATS_NAME (not option) */
  "*meters",             /* STATS_METERS (not option) */
  "*meter-id",           /* STATS_METER_ID (not option) */
  "*flow-count",         /* STATS_FLOW_COUNT (not option) */
  "*packet-in-count",    /* STATS_PACKET_IN_COUNT (not option) */
  "*byte-in-count",      /* STATS_BYTE_IN_COUNT (not option) */
  "*duration-sec",       /* STATS_DURATION_SEC (not option) */
  "*duration-nsec",      /* STATS_DURATION_NSEC (not option) */
  "*band-stats",         /* STATS_BAND (not option) */
  "*packet-band-count",  /* STATS_BAND_PACKET_BAND_COUNT (not option) */
  "*byte-band-count",    /* STATS_BAND_BYTE_BAND_COUNT (not option) */
  "*band-id",            /* STATS_BAND_ID (not option) */
};

/* dumps name. */
static const char *const dump_strs[DUMPS_MAX] = {
  "*name",               /* DUMPS_NAME (not option) */
  "*meters",             /* DUMPS_METERS (not option) */
  "*meter-id",           /* DUMPS_METER_ID (not option) */
  "*flags",              /* DUMPS_FLAGS (not option) */
  "*bands",              /* DUMPS_BANDS (not option) */
  "*type",               /* DUMPS_BAND_TYPE (not option) */
  "*rate",               /* DUMPS_BAND_RATE (not option) */
  "*burst-size",         /* DUMPS_BAND_BURST_SIZE (not option) */
  "*prec-level",         /* DUMPS_BAND_PREC_LEVEL (not option) */
  "*experimenter",       /* DUMPS_BAND_EXPERIMENTER (not option) */
  "*band-id",            /* DUMPS_BAND_ID (not option) */
};

/* flags name. */
static const char *const flag_strs[] = {
  "kbps",                /* OFPMF_KBPS */
  "pktps",               /* OFPMF_PKTPS */
  "burst",               /* OFPMF_BURS */
  "stats",               /* OFPMF_STATS */
};

static const size_t flag_strs_size =
    sizeof(flag_strs) / sizeof(const char *);

/* type num. */
enum mb_types {
  TYPE_DROP = 0,                /* OFPMBT_DROP */
  TYPE_DSCP_REMARK,             /* OFPMBT_DSCP_REMARK */
  TYPE_EXPERIMENTER,            /* OFPMBT_EXPERIMENTER */

  TYPE_MAX,
};

/* type name. */
static const char *const type_strs[TYPE_MAX] = {
  "drop",                /* TYPE_DROP */
  "dscp-remark",         /* TYPE_DSCP_REMARK */
  "ofpmbt-experimenter", /* TYPE_EXPERIMENTER */
};

typedef struct configs {
  bool is_dump;
  bool is_show_stats;
  datastore_bridge_meter_info_list_t dump_list;
  datastore_bridge_meter_stats_list_t stats_list;
} configs_t;

static lagopus_hashmap_t sub_cmd_table = NULL;

static inline void
meter_info_list_elem_free(datastore_bridge_meter_info_list_t *list) {
  struct meter_band *band = NULL;
  struct meter_config *info = NULL;

  while ((info = TAILQ_FIRST(list)) != NULL) {
    while ((band = TAILQ_FIRST(&info->band_list)) != NULL) {
      TAILQ_REMOVE(&info->band_list, band, entry);
      free(band);
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
        ret = dp_bridge_meter_stats_list_get(name, &configs->stats_list);
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
      ret = dp_bridge_meter_list_get(name, &configs->dump_list);
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
dump_type(uint32_t type) {
  switch(type) {
    case OFPMBT_DROP:
      return type_strs[TYPE_DROP];
      break;
    case OFPMBT_DSCP_REMARK:
      return type_strs[TYPE_DSCP_REMARK];
      break;
    case OFPMBT_EXPERIMENTER:
      return type_strs[TYPE_EXPERIMENTER];
      break;
    default:
      return "unknown";
      break;
  }
}

static lagopus_result_t
meter_cmd_dump_json_create(char *name,
                           lagopus_dstring_t *ds,
                           configs_t *configs,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct meter_band *band = NULL;
  struct meter_config *info = NULL;
  uint32_t band_id = 0;
  bool delimiter1;
  bool delimiter2;

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

      /* meters */
      if ((ret = DSTRING_CHECK_APPENDF(
              ds, true,
              KEY_FMT"[",
              ATTR_NAME_GET(dump_strs, DUMPS_METERS))) !=
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

          /* meter_id */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(dump_strs, DUMPS_METER_ID),
                  info->ofp.meter_id, false)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* flags */
          if ((ret = datastore_json_uint16_flags_append(
                  ds, ATTR_NAME_GET(dump_strs, DUMPS_FLAGS),
                  info->ofp.flags,
                  flag_strs, flag_strs_size, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* bands */
          if ((ret = DSTRING_CHECK_APPENDF(
                  ds, true,
                  KEY_FMT"[",
                  ATTR_NAME_GET(dump_strs, DUMPS_BANDS))) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if (TAILQ_EMPTY(&info->band_list) == false) {
            delimiter2 = false;
            TAILQ_FOREACH(band, &info->band_list, entry) {
              if ((ret = DSTRING_CHECK_APPENDF(
                      ds, delimiter2, "{")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* band_id */
              if ((ret = datastore_json_uint32_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BAND_ID),
                      band_id, false)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* type */
              if ((ret = datastore_json_string_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BAND_TYPE),
                      dump_type(band->type), true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* rate */
              if ((ret = datastore_json_uint32_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BAND_RATE),
                      band->rate, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* burst_size */
              if ((ret = datastore_json_uint32_append(
                      ds, ATTR_NAME_GET(dump_strs, DUMPS_BAND_BURST_SIZE),
                      band->burst_size, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* prec_level */
              if (band->type == OFPMBT_DSCP_REMARK) {
                if ((ret = datastore_json_uint8_append(
                        ds, ATTR_NAME_GET(dump_strs, DUMPS_BAND_PREC_LEVEL),
                        band->prec_level, true)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              }

              /* experimenter */
              if (band->type == OFPMBT_EXPERIMENTER) {
                if ((ret = datastore_json_uint32_append(
                        ds, ATTR_NAME_GET(dump_strs, DUMPS_BAND_EXPERIMENTER),
                      band->experimenter, true)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              }

              if ((ret = lagopus_dstring_appendf(ds, "}")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              if (delimiter2 == false) {
                delimiter2 = true;
              }

              band_id++;
            }

            if (delimiter1 == false) {
              delimiter1 = true;
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
meter_cmd_stats_json_create(char *name,
                            lagopus_dstring_t *ds,
                            configs_t *configs,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct meter_stats *stats = NULL;
  struct meter_band_stats *band_stats = NULL;
  uint32_t band_id = 0;
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

      /* meters */
      if ((ret = DSTRING_CHECK_APPENDF(
              ds, true,
              KEY_FMT"[",
              ATTR_NAME_GET(stat_strs, STATS_METERS))) !=
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

          /* meter_id */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_METER_ID),
                  stats->ofp.meter_id, false)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* flow_count */
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_FLOW_COUNT),
                  stats->ofp.flow_count, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* packet_in_count */
          if ((ret = datastore_json_uint64_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_PACKET_IN_COUNT),
                  stats->ofp.packet_in_count, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* byte_in_count */
          if ((ret = datastore_json_uint64_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_BYTE_IN_COUNT),
                  stats->ofp.byte_in_count, true)) !=
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

          /* band_stats */
          if ((ret = DSTRING_CHECK_APPENDF(
                  ds, true,
                  KEY_FMT"[",
                  ATTR_NAME_GET(stat_strs, STATS_BAND))) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          if (TAILQ_EMPTY(&stats->meter_band_stats_list) == false) {
            delimiter2 = false;
            TAILQ_FOREACH(band_stats, &stats->meter_band_stats_list, entry) {
              if ((ret = DSTRING_CHECK_APPENDF(
                      ds, delimiter2, "{")) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* band_id */
              if ((ret = datastore_json_uint64_append(
                      ds, ATTR_NAME_GET(stat_strs, STATS_BAND_ID),
                      band_id, false)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* packet_band_count */
              if ((ret = datastore_json_uint64_append(
                      ds, ATTR_NAME_GET(stat_strs, STATS_BAND_PACKET_BAND_COUNT),
                      band_stats->ofp.packet_band_count, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              /* byte_band_count */
              if ((ret = datastore_json_uint64_append(
                      ds, ATTR_NAME_GET(stat_strs, STATS_BAND_BYTE_BAND_COUNT),
                      band_stats->ofp.byte_band_count, true)) !=
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

              band_id++;
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
meter_cmd_parse(datastore_interp_t *iptr,
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
          ret = meter_cmd_stats_json_create(fullname, &conf_result, &out_configs,
                                            result);
        } else if (out_configs.is_dump){
          ret = meter_cmd_dump_json_create(fullname, &conf_result, &out_configs,
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
    meter_stats_list_elem_free(&out_configs.stats_list);
    meter_info_list_elem_free(&out_configs.dump_list);
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
             CMD_NAME, meter_cmd_parse)) !=
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
meter_cmd_initialize(void) {
  return initialize_internal();
}

void
meter_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
}
