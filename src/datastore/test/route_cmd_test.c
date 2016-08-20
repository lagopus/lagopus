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

#include "unity.h"
#include "cmd_test_utils.h"
#include "../route_cmd_internal.h"
#include "../bridge_cmd_internal.h"

#define TMP_DIR "/tmp"
#define TMP_FILE "lagopus_route_test"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;
static bool is_init = false;
static const char bridge_name[] = "br0";

void
setUp(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", bridge_name, "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* create interp. */
  INTERP_CREATE(ret, NULL,interp, tbl, ds);

  /* bridge create cmd. */
  if (is_init == false) {
    is_init = true;
    TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                   ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                   &ds, str, test_str1);
  }
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", bridge_name, "destroy",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  char path[PATH_MAX];
  DIR *dp = NULL;
  struct dirent *dirent = NULL;

  /* bridge destroy cmd. */
  if (destroy == true) {
    TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                   ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                   &ds, str, test_str1);
  }

  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy);

  /* delete file. */
  dp = opendir(TMP_DIR);
  if (dp != NULL) {
    while ((dirent = readdir(dp)) != NULL) {
      if (strncmp(dirent->d_name, TMP_FILE, strlen(TMP_FILE))) {
        sprintf(path, "TMP_DIR/%s", dirent->d_name);
        unlink(path);
      }
    }
    closedir(dp);
  }
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_dump_01(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         NULL
                        };
  const char *test_str1 = "";

  /* dump cmd */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_route_cmd_parse_dump_02(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "br0",
                         "-table-id", "1",
                         NULL
                        };
  const char test_str1[] = "";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_config_01(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "br0",
                         NULL
                        };
  const char test_str1[] = "";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_dump_not_bridge(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "bridge01",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"bridge01\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_dump_bad_opt_01(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "br0",
                         "-hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"opt = -hoge.\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_dump_bad_opt_02(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "br0",
                         "config",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"opt = config.\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_dump_bad_opt_val_01(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "br0",
                         "-table-id",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_cmd_parse_dump_bad_opt_val_02(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "br0",
                         "-table-id", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_cmd_parse_config_tmp_dir_01(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "config",
                         "-tmp-dir", "/tmp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_config_tmp_dir_02(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "config",
                         "-tmp-dir",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\",\n"
                           "\"data\":{\"tmp-dir\":\"\\/tmp\"}}";

  /* show cmd */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_config_tmp_dir03(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "config",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\",\n"
                           "\"data\":{\"tmp-dir\":\"\\/tmp\"}}";

  /* show cmd */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_config_bad_tmp_dir(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "config",
                         "-tmp-dir", "hoge",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"POSIX_API_ERROR\",\n"
                           "\"data\":\"Bad opt value = hoge.\"}";

  /* show cmd */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_parse_config_bad_opt(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"route",
                         "config",
                         "hoge",
                         NULL
                        };
  const char test_str1[] = "";

  /* show cmd */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_NOT_FOUND,
                 route_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_destroy(void) {
#ifdef HYBRID
  destroy = true;
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

