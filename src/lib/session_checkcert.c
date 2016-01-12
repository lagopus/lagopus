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

#include <regex.h>





typedef enum {
  conn_unknown = 0,
  conn_allow,
  conn_deny
} conn_permission_t;


typedef struct {
  conn_permission_t m_perm;
  const char *m_issuer;
  regex_t *m_issuer_rex;
  const char *m_subject;
  regex_t *m_subject_rex;
  int m_cmplen;
} pattern_pair_t;


typedef struct {
  pattern_pair_t **m_pats;
  size_t m_n_pats;
} serialize_arg;





static lagopus_mutex_t s_lock;

static lagopus_hashmap_t s_pat_tbl;
static pattern_pair_t **s_pats;
static size_t s_n_pats;

static char s_filename[PATH_MAX];
static time_t s_last_open;

static bool s_allow_selfsigned = false;


static inline void	s_unload_config(void);




static inline lagopus_result_t
s_compile(const char *pat, regex_t **retpat) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(pat) == true &&
      retpat != NULL) {
    regex_t rc;
    int st = regcomp(&rc, pat, REG_EXTENDED | REG_NOSUB);

    *retpat = NULL;

    if (st == 0) {
      /*
       * FIXME;
       *	I'm not sure that this won't cause a memory leak around
       *	the rc handling on all the regex implementations.
       */
      regex_t *rpat = (regex_t *)malloc(sizeof(rc));
      if (rpat != NULL) {
        (void)memcpy((void *)rpat, (void *)&rc, sizeof(rc));
        *retpat = rpat;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      char errbuf[4096];
      (void)regerror(st, (const regex_t *)&rc, errbuf, sizeof(errbuf));
      lagopus_msg_error("regcomp(\"%s\") error: %s\n", pat, errbuf);
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline pattern_pair_t *
s_create_pair(conn_permission_t perm,
              const char *issuer, const char *subject) {
  pattern_pair_t *ret = NULL;
  const char *pi = NULL;
  regex_t *ri = NULL;
  const char *ps = NULL;
  regex_t *rs = NULL;
  lagopus_result_t s;
  bool isok = false;
  int cmplen = 0;

  if (perm == conn_allow || perm == conn_deny ||
      IS_VALID_STRING(issuer) == true ||
      IS_VALID_STRING(subject) == true) {

    if (IS_VALID_STRING(issuer) == true) {
      if ((s = s_compile(issuer, &ri)) == LAGOPUS_RESULT_OK) {
        if ((pi = strdup(issuer)) != NULL) {
          cmplen += (int)strlen(pi);
        } else {
          goto done;
        }
      } else {
        lagopus_perror(s);
        goto done;
      }
    }
    if (IS_VALID_STRING(subject) == true) {
      if ((s = s_compile(subject, &rs)) == LAGOPUS_RESULT_OK) {
        if ((ps = strdup(subject)) != NULL) {
          cmplen += (int)strlen(ps);
        } else {
          goto done;
        }
      } else {
        lagopus_perror(s);
        goto done;
      }
    }

    if ((ret = (pattern_pair_t *)malloc(sizeof(*ret))) != NULL) {
      if (perm == conn_deny) {
        /*
         * Add an offset for the entry sort.
         */
        cmplen += 100000;
      }
      ret->m_perm = perm;
      ret->m_issuer = pi;
      ret->m_issuer_rex = ri;
      ret->m_subject = ps;
      ret->m_subject_rex = rs;
      ret->m_cmplen = cmplen;
      isok = true;
    } else {
      lagopus_perror(LAGOPUS_RESULT_NO_MEMORY);
    }
  } else {
    lagopus_perror(LAGOPUS_RESULT_INVALID_ARGS);
  }

done:
  if (isok == false) {
    free((void *)pi);
    if (ri != NULL) {
      regfree(ri);
      free((void *)ri);
    }
    free((void *)ps);
    if (rs != NULL) {
      regfree(rs);
      free((void *)rs);
    }
    free((void *)ret);
    ret = NULL;
  }

  return ret;
}


static inline void
s_destroy_pair(pattern_pair_t *p) {
  if (p != NULL) {
    free((void *)(p->m_issuer));
    free((void *)(p->m_subject));
    if (p->m_issuer_rex != NULL) {
      regfree(p->m_issuer_rex);
      free((void *)(p->m_issuer_rex));
    }
    if (p->m_subject_rex != NULL) {
      regfree(p->m_subject_rex);
      free((void *)(p->m_subject_rex));
    }
    free((void *)p);
  }
}


static inline conn_permission_t
s_check(pattern_pair_t *p, const char *issuer, const char *subject) {
  conn_permission_t ret = conn_unknown;

  if (IS_VALID_STRING(p->m_issuer) == true &&
      IS_VALID_STRING(p->m_subject) == true) {
    if (IS_VALID_STRING(issuer) == true &&
        IS_VALID_STRING(subject) == true) {
      if (regexec(p->m_issuer_rex, issuer, 0, NULL, 0) == 0 &&
          regexec(p->m_subject_rex, subject, 0, NULL, 0) == 0) {
        ret = p->m_perm;
      }
    }
  } else if (IS_VALID_STRING(p->m_issuer) == true) {
    if (IS_VALID_STRING(issuer) == true) {
      if (regexec(p->m_issuer_rex, issuer, 0, NULL, 0) == 0) {
        ret = p->m_perm;
      }
    }
  } else if (IS_VALID_STRING(p->m_subject) == true) {
    if (IS_VALID_STRING(subject) == true) {
      if (regexec(p->m_subject_rex, subject, 0, NULL, 0) == 0) {
        ret = p->m_perm;
      }
    }
  }

  return ret;
}





static inline void
s_free_pair(void *p) {
  s_destroy_pair((pattern_pair_t *)p);
}


static inline lagopus_result_t
s_create_tbl(void) {
  return lagopus_hashmap_create(&s_pat_tbl,
                                LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                s_free_pair);
}


static inline void
s_destroy_tbl(void) {
  lagopus_hashmap_destroy(&s_pat_tbl, true);
}


static inline lagopus_result_t
s_clear_tbl(void) {
  return lagopus_hashmap_clear(&s_pat_tbl, true);
}


static inline void
s_add_pair(pattern_pair_t *p) {
  void *v = (void *)p;
  (void)lagopus_hashmap_add(&s_pat_tbl, (void *)p, &v, true);
}


static bool
s_serilize_proc(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  bool ret = false;

  (void)val;
  (void)he;

  if (arg != NULL) {
    serialize_arg *sarg = (serialize_arg *)arg;
    pattern_pair_t **pats = sarg->m_pats;
    size_t n_pats = sarg->m_n_pats;

    if (pats != NULL) {
      pats[n_pats++] = (pattern_pair_t *)key;
      sarg->m_n_pats = n_pats;
      ret = true;
    }
  }

  return ret;
}


static int
s_cmp_proc(const void *v0, const void *v1, void *arg) {

  (void) arg;

  if (v0 != NULL && v1 != NULL) {
    pattern_pair_t **p0 = (pattern_pair_t **)v0;
    pattern_pair_t **p1 = (pattern_pair_t **)v1;
    int p0len = (*p0)->m_cmplen;
    int p1len = (*p1)->m_cmplen;

    /*
     * We need the longest match table.
     */
    return p1len - p0len;

  } else {
    return 0;
  }
}


static inline pattern_pair_t **
s_serialize_pairs(size_t *szptr) {
  pattern_pair_t **ret = NULL;
  size_t n_pats = 0;

  if (szptr != NULL) {
    *szptr = 0;
  }

  if ((n_pats = (size_t)lagopus_hashmap_size(&s_pat_tbl)) > 0) {
    ret = (pattern_pair_t **)malloc(sizeof(pattern_pair_t *) * (n_pats + 1));
    if (ret != NULL) {
      lagopus_result_t rc;
      serialize_arg s;

      (void)memset((void *)ret, 0, sizeof(pattern_pair_t *) * (n_pats + 1));
      s.m_pats = ret;
      s.m_n_pats = 0;

      if ((rc = lagopus_hashmap_iterate(&s_pat_tbl,
                                        s_serilize_proc,
                                        (void *)&s)) == LAGOPUS_RESULT_OK) {
        lagopus_qsort_r((void *)ret, n_pats, sizeof(pattern_pair_t *),
                        s_cmp_proc, NULL);
        *szptr = n_pats;
      }
    } else {
      lagopus_msg_error("no memory, n_pats %d, size %d\n",
                        (int) n_pats, (int) (sizeof(pattern_pair_t *) * (n_pats + 1)));
    }
  } else {
    lagopus_perror((lagopus_result_t) n_pats);
  }

  return ret;
}





static void
s_checkcert_init(void) {
  if (s_create_tbl() != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't initialize a hashmap for certificate check.\n");
  }
  if (lagopus_mutex_create(&s_lock) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't initialize a mutex for certificate check.\n");
  }
}


static void
s_checkcert_final(void) {
  s_unload_config();
  s_destroy_tbl();
  if (s_lock != NULL) {
    (void)lagopus_mutex_destroy(&s_lock);
  }
}





static inline time_t
s_get_mtime(const char *filename) {
  time_t ret = 0;

  if (IS_VALID_STRING(filename) == true) {
    struct stat st;
    if (stat(filename, &st) == 0) {
      ret = st.st_mtime;
    }
  }

  return ret;
}


static inline pattern_pair_t *
s_parse_line(const char *line, const char *filename, size_t lino) {
  pattern_pair_t *ret = NULL;
  char *buf = NULL;
  conn_permission_t perm = conn_unknown;
  char *issuer = NULL;
  char *subject = NULL;
  bool isok = false;
  bool do_create = false;

  if (IS_VALID_STRING(line) == true) {
    char *tokens[16];
    lagopus_result_t n_tokens;

    buf = strdup(line);
    if (buf == NULL) {
      goto done;
    }

    n_tokens = lagopus_str_tokenize_quote(buf, tokens, 1024,
                                          "\t\r\n ", "\"'");
    if (n_tokens > 0) {
      if (n_tokens < 16) {
        if (*(tokens[0]) != '#') {
          char **tp = tokens;

          tp[n_tokens] = NULL;

          if (n_tokens > 1) {
            if (strcasecmp(tp[0], "allow") == 0) {
              perm = conn_allow;
            } else if (strcasecmp(tp[0], "deny") == 0) {
              perm = conn_deny;
            }
            if (perm != conn_unknown) {

              tp++;
              while (*tp != NULL) {

                if (strcasecmp(*tp, "self-signed") == 0 ||
                    strcasecmp(*tp, "selfsigned") == 0) {

                  s_allow_selfsigned = (perm == conn_allow) ? true : false;

                } else if (strcasecmp(*tp, "issuer") == 0) {

                  if (IS_VALID_STRING(issuer) == false) {
                    tp++;
                    if (IS_VALID_STRING(*tp) == true) {
                      if (lagopus_str_unescape(*tp, "\"'", &issuer) > 0) {
                        do_create = true;
                      } else {
                        lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                          "Invalid escape: '%s'.\n",
                                          filename, lino, *tp);
                        goto done;
                      }
                    } else {
                      lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                        "Can't get an issuer pattern.\n",
                                        filename, lino);
                      goto done;
                    }
                  } else {
                    lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                      "The issuer pattern is already "
                                      "specified.\n",
                                      filename, lino);
                    goto done;
                  }

                } else if (strcasecmp(*tp, "subject") == 0) {

                  if (IS_VALID_STRING(subject) == false) {
                    tp++;
                    if (IS_VALID_STRING(*tp) == true) {
                      if (lagopus_str_unescape(*tp, "\"'", &subject) > 0) {
                        do_create = true;
                      } else {
                        lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                          "Invalid escape: '%s'.\n",
                                          filename, lino, *tp);
                        goto done;
                      }
                    } else {
                      lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                        "Can't get a subject pattern.\n",
                                        filename, lino);
                      goto done;
                    }
                  } else {
                    lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                      "The subject pattern is already "
                                      "specified.\n",
                                      filename, lino);
                    goto done;
                  }

                } else {

                  lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                    "Unknown/Invalid keyword: '%s'.\n",
                                    filename, lino, *tp);
                  goto done;

                }

                tp++;

              } /* end while */

            } else {
              lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                                "Unknown/Invalid permission: '%s'.\n",
                                filename, lino, tp[0]);
              goto done;
            }
          } else {
            lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                              "Insufficient tokens.\n",
                              filename, lino);
            goto done;
          }
        }
      } else {
        lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                          "Too many tokens.\n",
                          filename, lino);
        goto done;
      }
    } else {
      if (n_tokens < 0) {
        lagopus_perror(n_tokens);
        goto done;
      }
    }
  } else {
    goto done;
  }

  isok = true;

done:
  if (isok == true && do_create == true) {
    ret = s_create_pair(perm, issuer, subject);
  }

  free((void *)buf);
  free((void *)issuer);
  free((void *)subject);

  return ret;
}


static inline void
s_unload_config(void) {
  (void)s_clear_tbl();
  free(s_pats);
  s_pats = NULL;
  s_n_pats = 0;
}


static inline lagopus_result_t
s_load_config(const char *filename) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(filename) == true) {
    time_t mtime = s_get_mtime(filename);

    if (mtime > 0 &&
        ((s_pats == NULL || s_last_open == 0) ||
         (s_last_open < mtime))) {

      FILE *fd = fopen(filename, "r");
      if (fd != NULL) {
        char buf[4096];
        size_t lino = 0;
        pattern_pair_t *p;

        /*
         * load the config file.
         */

        s_unload_config();

        while (fgets(buf, sizeof(buf), fd) != NULL) {
          lino++;
          if (strlen(buf) >= (sizeof(buf) - 1)) {
            lagopus_msg_error("\"%s\": line " PFSZ(u) ": "
                              "The line too long.\n",
                              filename, lino);
            continue;
          }
          p = s_parse_line(buf, filename, lino);
          if (p != NULL) {
            s_add_pair(p);
          }
        }
        (void)fclose(fd);

        s_pats = s_serialize_pairs(&s_n_pats);
        if (s_pats != NULL) {
          s_last_open = time(NULL);
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }

      } else {
        ret = LAGOPUS_RESULT_NOT_FOUND;
      }
    } else if (mtime == 0) {
      ret = LAGOPUS_RESULT_NOT_FOUND;
    } else {
      /*
       * No need to load the file.
       */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret != LAGOPUS_RESULT_OK) {
    s_unload_config();
  }

  return ret;
}




static void
s_check_certificates_set_config_file(const char *filename) {
  (void)lagopus_mutex_lock(&s_lock);
  snprintf(s_filename, sizeof(s_filename), "%s", filename);
  s_last_open = 0;
  (void)lagopus_mutex_unlock(&s_lock);
}

static char *
s_check_certificates_get_config_file(void) {
  char *str = NULL;
  (void)lagopus_mutex_lock(&s_lock);
  if (s_filename != NULL) {
    str = strdup(s_filename);
  }
  (void)lagopus_mutex_unlock(&s_lock);

  return str;
}

static lagopus_result_t
s_check_certificates(const char *issuer_dn,
                     const char *subject_dn) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool has_issuer = IS_VALID_STRING(issuer_dn);
  bool has_subject = IS_VALID_STRING(subject_dn);

  if (has_issuer == true || has_subject == true) {
    bool is_selfsigned = false;
    (void)lagopus_mutex_lock(&s_lock);
    {
      if (IS_VALID_STRING(s_filename) == true) {
        if ((ret = s_load_config(s_filename)) != LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          lagopus_msg_error("Can't load: '%s'.\n", s_filename);
          /*
           * for failsafe.
           */
          s_allow_selfsigned = false;
        }
      }
    }
    (void)lagopus_mutex_unlock(&s_lock);

    /*
     * Check self-signed first.
     */
    if (has_issuer == true && has_subject == true &&
        strcmp(issuer_dn, subject_dn) == 0) {
      is_selfsigned = true;
    }
    if (s_allow_selfsigned == false && is_selfsigned == true) {
      ret = LAGOPUS_RESULT_NOT_ALLOWED;
      goto done;
    }

    if (ret == LAGOPUS_RESULT_OK) {
      ret = LAGOPUS_RESULT_NOT_ALLOWED;

      if (s_pats != NULL && s_n_pats > 0) {
        conn_permission_t perm = conn_unknown;
        pattern_pair_t *p;
        size_t i;

        for (i = 0; i < s_n_pats; i++) {
          p = s_pats[i];
          if (p != NULL) {
            perm = s_check(p, issuer_dn, subject_dn);
            if (perm != conn_unknown) {
              ret = (perm == conn_allow) ?
                    LAGOPUS_RESULT_OK : LAGOPUS_RESULT_NOT_ALLOWED;
              break;
            }
          }
        }

      }

    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

