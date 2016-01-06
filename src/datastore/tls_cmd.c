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
#include "lagopus_session_tls.h"
#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "conv_json.h"

#define TLS_CMD_NAME "tls"

#define DEFAULT_TLS_CERTIFICATE_STORE  CONFDIR
#define DEFAULT_TLS_CERTIFICATE_FILE   CONFDIR "/catls.pem"
#define DEFAULT_TLS_PRIVATE_KEY        CONFDIR "/key.pem"
#define DEFAULT_TLS_TRUST_POINT_CONF   CONFDIR "/check.conf"
#define RESULT_NULL	"\"\""




static inline lagopus_result_t
s_parse_tls_show(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *session_cert = NULL;
  char *private_key = NULL;
  char *session_ca_dir = NULL;
  char *trust_point_conf = NULL;
  char *esc_certfile = NULL;
  char *esc_pvtkey = NULL;
  char *esc_certstore = NULL;
  char *esc_tpconf = NULL;

  if ((ret = lagopus_session_tls_get_server_cert(&session_cert))
      == LAGOPUS_RESULT_OK) {
    ret = datastore_json_string_escape(session_cert,
                                       (char **)&esc_certfile);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't convert "
                                              "to an escaped string");
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get session_cert");
    goto done;
  }
  if ((ret = lagopus_session_tls_get_server_key(&private_key))
      == LAGOPUS_RESULT_OK) {
    ret = datastore_json_string_escape(private_key,
                                       (char **)&esc_pvtkey);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't convert "
                                              "to an escaped string");
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get private-key");
    goto done;
  }
  if ((ret = lagopus_session_tls_get_ca_dir(&session_ca_dir))
      == LAGOPUS_RESULT_OK) {
    ret = datastore_json_string_escape(session_ca_dir,
                                       (char **)&esc_certstore);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't convert "
                                              "to an escaped string");
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get certificate-store");
    goto done;
  }
  if ((ret = lagopus_session_tls_get_trust_point_conf(&trust_point_conf))
      == LAGOPUS_RESULT_OK) {
    ret = datastore_json_string_escape(trust_point_conf,
                                       (char **)&esc_tpconf);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't convert "
                                              "to an escaped string");
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get trust-point-conf");
    goto done;
  }

  ret = datastore_json_result_setf(result, ret, "[{"
                                   "\"cert-file\":\"%s\",\n"
                                   "\"private-key\":\"%s\",\n"
                                   "\"certificate-store\":\"%s\",\n"
                                   "\"trust-point-conf\":\"%s\""
                                   "}]",
                                   esc_certfile,
                                   esc_pvtkey,
                                   esc_certstore,
                                   esc_tpconf);

done:
  free((void *)esc_certfile);
  free((void *)esc_pvtkey);
  free((void *)esc_certstore);
  free((void *)esc_tpconf);
  free((void *)session_cert);
  free((void *)private_key);
  free((void *)session_ca_dir);
  free((void *)trust_point_conf);

  return ret;
}

static inline lagopus_result_t
s_parse_tls_internal(datastore_interp_state_t state,
                     const char *const argv[],
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *session_cert = NULL;
  char *private_key = NULL;
  char *session_ca_dir = NULL;
  char *trust_point_conf = NULL;
  char *esc_certfile = NULL;
  char *esc_pvtkey = NULL;
  char *esc_certstore = NULL;
  char *esc_tpconf = NULL;

  while (IS_VALID_STRING(*argv) == true) {

    if (strcmp(*argv, "-cert-file") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        ret = lagopus_str_unescape(*argv, "\"'", &session_cert);
        if (ret > 0) {
          if (state == DATASTORE_INTERP_STATE_DRYRUN) {
            ret = LAGOPUS_RESULT_OK;
          } else {
            if ((ret = lagopus_session_tls_set_server_cert(session_cert))
                != LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "Can't set %s.",
                                                      session_cert);
              goto done;
            }
          }
        } else if (ret == 0) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt val = %s ",
                                                  *argv);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert to an "
                                                  "unescaped strings.");
          goto done;
        }
      } else {

        ret = lagopus_session_tls_get_server_cert(&session_cert);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_string_escape(session_cert,
                                             (char **)&esc_certfile);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_setf(result, ret, "[{"
                                             "\"cert-file\":\"%s\""
                                             "}]", esc_certfile);
            goto done;

          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't convert to an "
                                                    "escaped strings.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get session_cert");
          goto done;
        }
      }

    } else if (strcmp(*argv, "-private-key") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        if ((ret = lagopus_str_unescape(*argv, "\"'", &private_key)) > 0) {
          if (state == DATASTORE_INTERP_STATE_DRYRUN) {
            ret = LAGOPUS_RESULT_OK;
          } else {
            if ((ret = lagopus_session_tls_set_server_key(private_key))
                == LAGOPUS_RESULT_OK) {
            } else {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "Can't set private_key.");
              goto done;
            }
          }
        } else if (ret == 0) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt val = %s.",
                                                  *argv);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert to an "
                                                  "unescaped strings.");
          goto done;
        }
      } else {
        ret = lagopus_session_tls_get_server_key(&private_key);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_string_escape(private_key,
                                             (char **)&esc_pvtkey);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_setf(result, ret, "[{"
                                             "\"private-key\":\"%s\""
                                             "}]", esc_pvtkey);
            goto done;
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't convert to an "
                                                    "escaped strings.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get private_key");
          goto done;
        }
      }

    } else if (strcmp(*argv, "-certificate-store") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        ret = lagopus_str_unescape(*argv, "\"'", &session_ca_dir);
        if (ret > 0) {
          if (state == DATASTORE_INTERP_STATE_DRYRUN) {
            ret = LAGOPUS_RESULT_OK;
          } else {
            if ((ret = lagopus_session_tls_set_ca_dir(session_ca_dir))
                == LAGOPUS_RESULT_OK) {
            } else {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "Can't set "
                                                      "certificate-store.");
              goto done;
            }
          }
        } else if (ret == 0) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt val = %s ",
                                                  *argv);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert to an "
                                                  "unescaped strings.");
          goto done;
        }
      } else {
        ret = lagopus_session_tls_get_ca_dir(&session_ca_dir);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_string_escape(session_ca_dir,
                                             (char **)&esc_certstore);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_setf(result, ret, "[{"
                                             "\"certificate-store\":\"%s\""
                                             "}]", esc_certstore);
            goto done;
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't convert to an "
                                                    "escaped strings.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get "
                                                  "certificate-store");
          goto done;
        }
      }

    } else if (strcmp(*argv, "-trust-point-conf") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        ret = lagopus_str_unescape(*argv, "\"'", &trust_point_conf);
        if (ret > 0) {
          if (state == DATASTORE_INTERP_STATE_DRYRUN) {
            ret = LAGOPUS_RESULT_OK;
          } else {
            if ((ret = lagopus_session_tls_set_trust_point_conf(trust_point_conf))
                == LAGOPUS_RESULT_OK) {
            } else {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "Can't set "
                                                      "trust-point-conf.");
              goto done;
            }
          }
        } else if (ret == 0) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt val = %s ",
                                                  *argv);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert to an "
                                                  "unescaped strings.");
          goto done;
        }

      } else {
        ret = lagopus_session_tls_get_trust_point_conf(&trust_point_conf);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_string_escape(trust_point_conf,
                                             (char **)&esc_tpconf);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_setf(result, ret, "[{"
                                             "\"trust-point-conf\":\"%s\""
                                             "}]", esc_tpconf);
            goto done;
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't convert to an "
                                                    "escaped strings.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get "
                                                  "trust-point-conf");
          goto done;
        }
      }

    } else {
      char *esc_str = NULL;
      ret = datastore_json_string_escape(*argv, (char **)&esc_str);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_NOT_FOUND,
                                                "option = %s",
                                                esc_str);
      }
      free((void *)esc_str);
    }

 done:
    free((void *)session_cert);
    free((void *)private_key);
    free((void *)session_ca_dir);
    free((void *)trust_point_conf);
    free((void *)esc_certfile);
    free((void *)esc_pvtkey);
    free((void *)esc_certstore);
    free((void *)esc_tpconf);
    session_cert = NULL;
    private_key = NULL;
    session_ca_dir = NULL;
    trust_point_conf = NULL;
    esc_certfile = NULL;
    esc_pvtkey = NULL;
    esc_certstore = NULL;
    esc_tpconf = NULL;

    if (ret != LAGOPUS_RESULT_OK) {
      break;
    }
    argv++;
  }

  if (ret == LAGOPUS_RESULT_OK &&
      lagopus_dstring_empty(result)) {
    ret = lagopus_dstring_appendf(result, "{\"ret\":\"OK\"}");
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add strings "
                                              "to dstring.");
    }
  }

  return ret;
}

static lagopus_result_t
s_parse_tls(datastore_interp_t *iptr,
            datastore_interp_state_t state,
            size_t argc, const char *const argv[],
            lagopus_hashmap_t *hptr,
            datastore_update_proc_t u_proc,
            datastore_enable_proc_t e_proc,
            datastore_serialize_proc_t s_proc,
            datastore_destroy_proc_t d_proc,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  (void)iptr;
  (void)argc;
  (void)hptr;
  (void)u_proc;
  (void)e_proc;
  (void)s_proc;
  (void)d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  argv++;
  if (IS_VALID_STRING(*argv) == false) {
    ret = s_parse_tls_show(result);
  } else {
    ret = s_parse_tls_internal(state, argv, result);
  }

  return ret;
}

lagopus_result_t
tls_cmd_serialize(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* cert-file */
  char *cert_file_str = NULL;
  char *escaped_cert_file_str = NULL;

  /* private-key */
  char *private_key_str = NULL;
  char *escaped_private_key_str = NULL;

  /* certificate-store */
  char *certificate_store_str = NULL;
  char *escaped_certificate_store_str = NULL;

  /* trust-point-conf */
  char *trust_point_conf_str = NULL;
  char *escaped_trust_point_conf_str = NULL;

  bool is_escaped = false;

  if (result != NULL) {
    /* cmmand name. */
    if ((ret = lagopus_dstring_appendf(result, TLS_CMD_NAME)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* cert-file opt. */
    if ((ret = lagopus_session_tls_get_server_cert(&cert_file_str))
        == LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(cert_file_str) == true) {
        if ((ret = lagopus_str_escape(cert_file_str, "\"",
                                      &is_escaped,
                                      &escaped_cert_file_str)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " -cert-file")) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_cert_file_str),
                         escaped_cert_file_str)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          } else {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* private-key opt. */
    if ((ret = lagopus_session_tls_get_server_key(&private_key_str))
        == LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(private_key_str) == true) {
        if ((ret = lagopus_str_escape(private_key_str, "\"",
                                      &is_escaped,
                                      &escaped_private_key_str)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " -private-key")) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_private_key_str),
                         escaped_private_key_str)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          } else {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* certificate-store opt. */
    if ((ret = lagopus_session_tls_get_ca_dir(&certificate_store_str))
        == LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(certificate_store_str) == true) {
        if ((ret = lagopus_str_escape(certificate_store_str, "\"",
                                      &is_escaped,
                                      &escaped_certificate_store_str)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " -certificate-store")) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_certificate_store_str),
                         escaped_certificate_store_str)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          } else {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* trust-point-conf opt. */
    if ((ret = lagopus_session_tls_get_trust_point_conf(&trust_point_conf_str))
        == LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(trust_point_conf_str) == true) {
        if ((ret = lagopus_str_escape(trust_point_conf_str, "\"",
                                      &is_escaped,
                                      &escaped_trust_point_conf_str)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " -trust-point-conf")) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_trust_point_conf_str),
                         escaped_trust_point_conf_str)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          } else {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    free(cert_file_str);
    free(escaped_cert_file_str);
    free(private_key_str);
    free(escaped_private_key_str);
    free(certificate_store_str);
    free(escaped_certificate_store_str);
    free(trust_point_conf_str);
    free(escaped_trust_point_conf_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





#undef RESULT_NULL



