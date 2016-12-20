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

#ifndef __CMD_TEST_UTILS_H__
#define __CMD_TEST_UTILS_H__

#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/ofp_dp_apis.h"
#include "../../agent/channel.h"
#include "../../agent/channel_mgr.h"

#define ARGV_SIZE(_argv) ((sizeof(_argv) / sizeof(_argv[0])) - 1)

#define INTERP_CREATE(_ret, _name,_interp, _tbl, _ds) {                 \
    if (_interp == NULL) {                                              \
      const char *argv0 =                                               \
         ((IS_VALID_STRING(lagopus_get_command_name()) == true) ?       \
             lagopus_get_command_name() : "callout_test");              \
              const char * const argv[] = { argv0, NULL };              \
      _ret = datastore_create_interp(&(_interp));                       \
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,                \
                                "datastore_create_interp error.");      \
      _ret = datastore_all_commands_initialize();                       \
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,                \
                                "allcommands initialize error.");       \
      TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);              \
      ofp_bridgeq_mgr_initialize(NULL);                                 \
      (void)lagopus_mainloop_set_callout_workers_number(1);             \
      _ret = lagopus_mainloop_with_callout(1, argv, NULL, NULL,         \
                                            false, false, true);        \
      TEST_ASSERT_EQUAL(_ret, LAGOPUS_RESULT_OK);                       \
      channel_mgr_initialize();                                         \
    }                                                                   \
    if (_tbl == NULL) {                                                 \
      if (_name == NULL) {                                              \
        _ret = lagopus_hashmap_create(&(_tbl),                          \
                                      LAGOPUS_HASHMAP_TYPE_STRING,      \
                                      NULL);                            \
        TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,              \
                                  "lagopus_hashmap_create error.");     \
      } else {                                                          \
        _ret = datastore_find_table(_name, &(_tbl),                     \
                                    NULL, NULL, NULL,                   \
                                    NULL, NULL, NULL, NULL);            \
        TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,              \
                                  "datastore_find_table error.");       \
      }                                                                 \
    }                                                                   \
    if (_ds == NULL) {                                                  \
      _ret = lagopus_dstring_create(&(_ds));                            \
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,                 \
                                "lagopus_dstring_create error.");       \
    }                                                                   \
  }

#define INTERP_DESTROY(_name, _interp, _tbl, _ds,                       \
                       _destroy) {                                      \
    if (_interp != NULL && destroy == true) {                           \
      lagopus_result_t _ret;                                            \
      datastore_destroy_interp(&(_interp));                             \
      datastore_all_commands_finalize();                                \
      channel_mgr_finalize();                                           \
      dp_api_fini();                                                    \
      ofp_bridgeq_mgr_destroy();                                        \
      _ret = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);        \
      TEST_ASSERT_EQUAL(_ret, LAGOPUS_RESULT_OK);                       \
      lagopus_mainloop_wait_thread();                                   \
    }                                                                   \
    if (_tbl != NULL) {                                                 \
      if (_name == NULL) {                                              \
        if (destroy == true) {                                          \
          lagopus_hashmap_destroy(&(_tbl), true);                       \
        } else {                                                        \
          lagopus_hashmap_clear(&(_tbl), true);                         \
        }                                                               \
      }                                                                 \
    }                                                                   \
    if (_ds != NULL) {                                                  \
      lagopus_dstring_destroy(&(_ds));                                  \
    }                                                                   \
  }

#define TEST_JSON_FORMAT(_json){                                        \
    FILE *_fp = NULL;                                                   \
    int _ret = -1;                                                      \
    _fp = popen("jq '.' > /dev/null 2>&1", "w");                        \
    if (_fp != NULL){                                                   \
      _ret = fputs(_json, _fp);                                         \
      fflush(_fp);                                                      \
      TEST_ASSERT_EQUAL_MESSAGE(1, _ret >= 0,                           \
                                "fput error.");                         \
      _ret = pclose(_fp);                                               \
      TEST_ASSERT_EQUAL_MESSAGE(0, _ret,                                \
                                "json format error.");                  \
    } else {                                                            \
      TEST_FAIL_MESSAGE("popen error.");                                \
    }                                                                   \
  }

#define TEST_DSTRING(_ret, _ds, _str, _test_str, _is_json) {            \
    _ret = lagopus_dstring_str_get((_ds), &_str);                       \
    printf("%s\n", _str);                                               \
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,                  \
                              "lagopus_dstring_str_get error.");        \
    if (_str != NULL) {                                                 \
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(_test_str, _str),             \
                                "string compare error.");               \
      if (_is_json == true) {                                           \
        TEST_JSON_FORMAT(_str);                                         \
      }                                                                 \
    } else {                                                            \
      TEST_FAIL_MESSAGE("string is null.");                             \
    }                                                                   \
    if (_str != NULL){                                                  \
      free(_str);                                                       \
      _str = NULL;                                                      \
    }                                                                   \
  }

#define TEST_DSTRING_JSON(_ret, _ds, _str) {                            \
    _ret = lagopus_dstring_str_get((_ds), &_str);                       \
    printf("%s\n", _str);                                               \
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,                  \
                              "lagopus_dstring_str_get error.");        \
    if (_str != NULL) {                                                 \
        TEST_JSON_FORMAT(_str);                                         \
    } else {                                                            \
      TEST_FAIL_MESSAGE("string is null.");                             \
    }                                                                   \
    if (_str != NULL){                                                  \
      free(_str);                                                       \
      _str = NULL;                                                      \
    }                                                                   \
  }

#define TEST_DSTRING_NO_JSON(_ret, _ds, _str, _test_str, _is_json) {    \
    _ret = lagopus_dstring_str_get((_ds), &_str);                       \
    printf("%s\n", _str);                                               \
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,                  \
                              "lagopus_dstring_str_get error.");        \
    if (_str != NULL) {                                                 \
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(_test_str, _str),             \
                                "string compare error.");               \
    } else {                                                            \
      TEST_FAIL_MESSAGE("string is null.");                             \
    }                                                                   \
    if (_str != NULL){                                                  \
      free(_str);                                                       \
      _str = NULL;                                                      \
    }                                                                   \
  }

#define TEST_CMD_PARSE(_ret, _cmp_ret, _parse_func, _interp, _state ,   \
                       _argc, _argv, _tbl, _update_func, _ds,           \
                       _str, _test_str) {                               \
  lagopus_dstring_clear(_ds);                                         \
  _ret = _parse_func(_interp, _state, _argc, _argv, _tbl,             \
                     _update_func, NULL, NULL, NULL, _ds);            \
  TEST_ASSERT_EQUAL_MESSAGE(_cmp_ret, _ret,                           \
                            #_parse_func " error.");                  \
  TEST_DSTRING(_ret, _ds, _str, _test_str, true);                     \
}

#define TEST_CMD_PARSE_WITHOUT_STRCMP(_ret, _cmp_ret, _parse_func,      \
                                      _interp, _state ,                 \
                                      _argc, _argv, _tbl,               \
                                      _update_func, _ds,                \
                                      _str) {                           \
  lagopus_dstring_clear(_ds);                                         \
  _ret = _parse_func(_interp, _state, _argc, _argv, _tbl,             \
                     _update_func, NULL, NULL, NULL, _ds);            \
  TEST_ASSERT_EQUAL_MESSAGE(_cmp_ret, _ret,                           \
                            #_parse_func " error.");                  \
  TEST_DSTRING_JSON(_ret, _ds, _str);                                 \
}

#define TEST_PROC(_ret, _cmp_ret, _func, _interp, _state, _tbl,         \
                  _name, _obj, _ds) {                                   \
  lagopus_dstring_clear(_ds);                                         \
  _ret = lagopus_hashmap_find(_tbl, (void *)_name, &_obj);            \
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,                  \
                            "lagopus_hashmap_find error.");           \
  _ret = _func(_interp, _state, _obj, _ds);                           \
  TEST_ASSERT_EQUAL_MESSAGE(_cmp_ret, _ret,                           \
                            #_func " error.");                        \
}

#define TEST_CMD_PROC(_ret, _cmp_ret, _func, _interp, _state, _tbl,     \
                      _name, _obj, _ds, _str, _test_str) {              \
  TEST_PROC(_ret, _cmp_ret, _func, _interp, _state, _tbl,             \
            _name, _obj, _ds);                                        \
  TEST_DSTRING(_ret, _ds, _str, _test_str, false);                    \
}

#define TEST_CMD_FLOW_DUMP(_ret, _cmp_ret, _bri_name, _table_id,        \
                           _ds, _str, _test_str) {                      \
    lagopus_dstring_clear(_ds);                                         \
    _ret = dump_bridge_domains_flow(DATASTORE_NAMESPACE_DELIMITER       \
                                    _bri_name, _table_id, false,        \
                                    true, _ds);                         \
    TEST_ASSERT_EQUAL_MESSAGE(_cmp_ret, _ret,                           \
                              "flow_cmd_dump error.");                  \
    TEST_DSTRING(_ret, _ds, _str, _test_str, true);                     \
  }


#define TEST_CONTROLLER_CREATE(_ret, _interp, _state, _tbl,             \
                               _ds, _str, _c_name, _ctrler_name) {      \
  const char *_c1[] = {"channel", _c_name, "create",                  \
                       "-dst-addr", "127.0.0.1",                      \
                       "-dst-port", "3000",                           \
                       "-local-addr", "127.0.0.1",                    \
                       "-local-port", "3000",                         \
                       "-protocol", "tcp",                            \
                       NULL};                                         \
  const char _c_str1[] = "{\"ret\":\"OK\"}";                          \
  const char *_ctrler1[] = {"controller", _ctrler_name, "create",     \
                            "-channel", _c_name,                      \
                            "-role", "equal",                         \
                            "-connection-type", "main",               \
                            NULL};                                    \
  const char _ctrler_str1[] = "{\"ret\":\"OK\"}";                     \
  /* channel create cmd. */                                           \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, channel_cmd_parse, _interp, \
                 _state, ARGV_SIZE(_c1), _c1, _tbl,                   \
                 channel_cmd_update,_ds, str, _c_str1);               \
  /* controller create cmd. */                                        \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, controller_cmd_parse,       \
                 _interp, _state, ARGV_SIZE(_ctrler1), _ctrler1,      \
                 _tbl, controller_cmd_update, _ds,                    \
                 str, _ctrler_str1);                                  \
}

#define TEST_CONTROLLER_DESTROY(_ret, _interp, _state, _tbl,            \
                                _ds, _str, _c_name, _ctrler_name) {     \
  const char *_c1[] = {"channel", _c_name, "destroy",                 \
                       NULL};                                         \
  const char _c_str1[] = "{\"ret\":\"OK\"}";                          \
  const char *_ctrler1[] = {"controller", _ctrler_name, "destroy",    \
                            NULL};                                    \
  const char _ctrler_str1[] = "{\"ret\":\"OK\"}";                     \
  /* controller destroy cmd. */                                       \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, controller_cmd_parse,       \
                 _interp, _state, ARGV_SIZE(_ctrler1), _ctrler1,      \
                 _tbl, controller_cmd_update, _ds,                    \
                 str, _ctrler_str1);                                  \
  /* channel destroy cmd. */                                          \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, channel_cmd_parse, _interp, \
                 _state, ARGV_SIZE(_c1), _c1, _tbl,                   \
                 channel_cmd_update, _ds, str, _c_str1);              \
}

#define TEST_PORT_CREATE(_ret, _interp, _state, _tbl,                   \
                         _ds, _str, _inter_name, _port_name) {          \
  const char *_inter1[] = {"interface", _inter_name, "create",        \
                           "-type", "ethernet-rawsock",               \
                           "-device", _inter_name,                    \
                           NULL};                                     \
  const char _inter_str1[] = "{\"ret\":\"OK\"}";                      \
  const char *_port1[] = {"port", _port_name, "create",               \
                          "-interface", _inter_name,                  \
                          NULL};                                      \
  const char _port_str1[] = "{\"ret\":\"OK\"}";                       \
  /* interface create cmd. */                                         \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, interface_cmd_parse,        \
                 _interp, _state, ARGV_SIZE(_inter1), _inter1, _tbl,  \
                 interface_cmd_update, _ds,                           \
                 _str, _inter_str1);                                  \
  /* port create cmd. */                                              \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, port_cmd_parse, _interp,    \
                 _state, ARGV_SIZE(_port1), _port1, _tbl,             \
                 port_cmd_update, _ds, _str, _port_str1);             \
}

#define TEST_PORT_DESTROY(_ret, _interp, _state, _tbl,                  \
                          _ds, _str, _inter_name, _port_name) {         \
  const char *_inter1[] = {"interface", _inter_name, "destroy",       \
                           NULL};                                     \
  const char _inter_str1[] = "{\"ret\":\"OK\"}";                      \
  const char *_port1[] = {"port", _port_name, "destroy",              \
                          NULL};                                      \
  const char _port_str1[] = "{\"ret\":\"OK\"}";                       \
  /* port destroy cmd. */                                             \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, port_cmd_parse, _interp,    \
                 _state, ARGV_SIZE(_port1), _port1, _tbl,             \
                 port_cmd_update, _ds, _str, _port_str1);             \
  /* interface destroy cmd. */                                        \
  TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, interface_cmd_parse,        \
                 _interp, _state, ARGV_SIZE(_inter1), _inter1, _tbl,  \
                 interface_cmd_update, _ds,                           \
                 _str, _inter_str1);                                  \
}

#define TEST_BRIDGE_CREATE(_ret, _interp, _state, _tbl,                 \
                           _ds, _str, _name, _dpid,                     \
                           _c_name, _ctrler_name,                       \
                           _inter_name, _port_name,                     \
                           _port_no) {                                  \
    const char *_b[] = {"bridge", _name, "create",                      \
                        "-controller", _ctrler_name,                    \
                        "-port", _port_name, _port_no,                  \
                        "-dpid", _dpid,                                 \
                        NULL};                                          \
    const char _b_str[] = "{\"ret\":\"OK\"}";                           \
    TEST_CONTROLLER_CREATE(_ret, _interp, _state, _tbl,                 \
                           _ds, _str, _c_name, _ctrler_name);           \
    TEST_PORT_CREATE(_ret, _interp, _state, _tbl,                       \
                     _ds, _str, _inter_name, _port_name);               \
    /* bridge create cmd. */                                            \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,           \
                   _interp, _state, ARGV_SIZE(_b), _b,                  \
                   _tbl, bridge_cmd_update, _ds,                        \
                   str, _b_str);                                        \
  }

#define TEST_BRIDGE_DESTROY(_ret, _interp, _state, _tbl,                \
                            _ds, _str, _name,                           \
                            _c_name, _ctrler_name,                      \
                            _inter_name, _port_name) {                  \
    const char *_b[] = {"bridge", _name, "destroy",                     \
                        NULL};                                          \
    const char _b_str[] = "{\"ret\":\"OK\"}";                           \
    /* bridge destroy cmd. */                                           \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,           \
                   _interp, _state, ARGV_SIZE(_b), _b,                  \
                   _tbl, bridge_cmd_update, _ds,                        \
                   str, _b_str);                                        \
    TEST_CONTROLLER_DESTROY(_ret, _interp, _state, _tbl,                \
                            _ds, _str, _c_name, _ctrler_name);          \
    TEST_PORT_DESTROY(_ret, _interp, _state, _tbl,                      \
                      _ds, _str, _inter_name, _port_name);              \
  }

#define TEST_QUEUE_CREATE(_ret, _interp, _state, _tbl,                  \
                          _ds, _str, _name) {                           \
    const char *_q[] = {"queue", _name, "create",                       \
                        "-type", "single-rate",                         \
                        NULL};                                          \
    const char _q_str[] = "{\"ret\":\"OK\"}";                           \
    /* queue create cmd. */                                             \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, queue_cmd_parse,            \
                   _interp, _state, ARGV_SIZE(_q), _q,                  \
                   _tbl, queue_cmd_update, _ds,                         \
                   str, _q_str);                                        \
  }

#define TEST_QUEUE_DESTROY(_ret, _interp, _state, _tbl,                 \
                           _ds, _str, _name) {                          \
    const char *_q[] = {"queue", _name, "destroy",                      \
                        NULL};                                          \
    const char _q_str[] = "{\"ret\":\"OK\"}";                           \
    /* queue destroy cmd. */                                            \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, queue_cmd_parse,            \
                   _interp, _state, ARGV_SIZE(_q), _q,                  \
                   _tbl, queue_cmd_update, _ds,                         \
                   str, _q_str);                                        \
  }

#define TEST_POLICER_CREATE(_ret, _interp, _state, _tbl,                \
                            _ds, _str, _pa_name, _name) {               \
    const char *_pa[] = {"policer-action", _pa_name, "create",          \
                         "-type", "discard",                            \
                        NULL};                                          \
    const char _pa_str[] = "{\"ret\":\"OK\"}";                          \
    const char *_p[] = {"policer", _name, "create",                     \
                        "-action", _pa_name,                            \
                        NULL};                                          \
    const char _p_str[] = "{\"ret\":\"OK\"}";                           \
    /* policer action create cmd. */                                    \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,   \
                   _interp, _state, ARGV_SIZE(_pa), _pa,                \
                   _tbl, policer_action_cmd_update, _ds,                \
                   str, _pa_str);                                       \
    /* policer create cmd. */                                           \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, policer_cmd_parse,          \
                   _interp, _state, ARGV_SIZE(_p), _p,                  \
                   _tbl, policer_cmd_update, _ds,                       \
                   str, _p_str);                                        \
  }

#define TEST_POLICER_DESTROY(_ret, _interp, _state, _tbl,               \
                             _ds, _str, _pa_name, _name) {              \
    const char *_pa[] = {"policer-action", _pa_name, "destroy",         \
                         NULL};                                         \
    const char _pa_str[] = "{\"ret\":\"OK\"}";                          \
    const char *_p[] = {"policer", _name, "destroy",                    \
                        NULL};                                          \
    const char _p_str[] = "{\"ret\":\"OK\"}";                           \
    /* policer destroy cmd. */                                          \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, policer_cmd_parse,          \
                   _interp, _state, ARGV_SIZE(_p), _p,                  \
                   _tbl, policer_cmd_update, _ds,                       \
                   str, _p_str);                                        \
    /* policer-action destroy cmd. */                                   \
    TEST_CMD_PARSE(_ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,   \
                   _interp, _state, ARGV_SIZE(_pa), _pa,                \
                   _tbl, policer_action_cmd_update, _ds,                \
                   str, _pa_str);                                       \
  }

#define TEST_ASSERT_FLOW_ADD(_ret, _dpid, _fm, _ml, _il, _er) {         \
    (_fm)->command = OFPFC_ADD;                                         \
    _ret = ofp_flow_mod_check_add((_dpid), (_fm), (_ml), (_il), (_er)); \
    TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);                          \
  }

#define TEST_ASSERT_FLOW_DEL(_ret, _dpid, _fm, _ml, _er) {            \
    (_fm)->command = OFPFC_DELETE;                                    \
    _ret = ofp_flow_mod_delete((_dpid), (_fm), (_ml), (_er));         \
    TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);                        \
  }

#define TEST_ASSERT_GROUP_ADD(_ret, _dpid, _gm, _bl, _er) {             \
    (_gm)->command = OFPGC_ADD;                                         \
    _ret = ofp_group_mod_add((_dpid), (_gm), (_bl), (_er));             \
    TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);                          \
  }

#define TEST_ASSERT_GROUP_DEL(_ret, _dpid, _gm, _er) {                  \
    (_gm)->command = OFPGC_DELETE;                                      \
    _ret = ofp_group_mod_delete((_dpid), (_gm), (_er));                 \
    TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);                          \
  }

#define TEST_ASSERT_METER_ADD(_ret, _dpid, _mm, _bl, _er) {             \
    (_mm)->command = OFPMC_ADD;                                         \
    _ret = ofp_meter_mod_add((_dpid), (_mm), (_bl), (_er));             \
    TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);                          \
  }

#define TEST_ASSERT_METER_DEL(_ret, _dpid, _mm, _er) {                  \
    (_mm)->command = OFPMC_DELETE;                                      \
    _ret = ofp_meter_mod_delete((_dpid), (_mm), (_er));                 \
    TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);                          \
  }

#endif /* __CMD_TEST_UTILS_H__ */
