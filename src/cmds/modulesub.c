/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofp_handler.h"
#ifdef ENABLE_SNMP_MODULE
# include "lagopus/snmpmgr.h"
#endif /* ENABLE_SNMP_MODULE */
#include "agent.h"
#include "confsys.h"





static struct dpmgr *s_dpmptr = NULL;





static inline char **
s_dup_argv(int argc, const char *const argv[]) {
  char **ret = (char **)malloc(sizeof(char *) * (size_t)(argc + 1));
  if (ret != NULL) {
    int i;

    (void)memset((void *)ret, 0, sizeof(char *) * (size_t)(argc + 1));

    for (i = 0; i < argc; i++) {
      ret[i] = strdup(argv[i]);
      if (ret[i] == NULL) {
        int j;
        for (j = 0; j < i; j++) {
          free((void *)ret[j]);
        }
        free((void *)ret);
        return ret;
      }
    }
  }
  return ret;
}


static inline void
s_free_argv(char **argv) {
  if (argv != NULL) {
    char **org = argv;
    while (*argv != NULL) {
      free((void *)*argv);
      argv++;
    }
    free((void *)org);
  }
}





static lagopus_result_t
config_handle_initialize_wrap(int argc, const char *const argv[],
                              void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;

  return config_handle_initialize(NULL, retptr);
}

static lagopus_result_t
agent_initialize_wrap(int argc, const char *const argv[],
                      void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;

  if (s_dpmptr != NULL) {
    return agent_initialize((void *)s_dpmptr, retptr);
  } else {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
}


static inline void
agent_finalize_wrap(void) {
  agent_finalize();
  if (s_dpmptr != NULL) {
    dpmgr_free(s_dpmptr);
    s_dpmptr = NULL;
  }
}

static lagopus_result_t
ofp_handler_initialize_wrap(int argc, const char *const argv[],
                            void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;
  (void)retptr;

  return ofp_handler_initialize(NULL, NULL);
}


static inline void
ofp_handler_finalize_wrap(void) {
  ofp_handler_finalize();
}

#ifdef ENABLE_SNMP_MODULE
static lagopus_result_t
snmpmgr_initialize_wrap(int argc, const char *const argv[],
                        void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;

  return snmpmgr_initialize((void *)true, retptr);
}
#endif /* ENABLE_SNMP_MODULE */





#define CTOR_IDX	LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 1


static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static void	s_ctors(void) __attr_constructor__(CTOR_IDX);
static void	s_dtors(void) __attr_destructor__(CTOR_IDX);


static void
s_once_proc(void) {
  lagopus_result_t r;
  const char *name;

  if ((s_dpmptr = dpmgr_alloc()) == NULL) {
    lagopus_exit_fatal("can't allocate a datapath managr.\n");
  }

  name = "config_handle";
  if ((r = lagopus_module_register(name,
                                   config_handle_initialize_wrap, NULL,
                                   config_handle_start,
                                   config_handle_shutdown,
                                   config_handle_stop,
                                   config_handle_finalize,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

  name = "datapath";
  if ((r = lagopus_module_register(name,
                                   datapath_initialize, s_dpmptr,
                                   datapath_start,
                                   datapath_shutdown,
                                   datapath_stop,
                                   datapath_finalize,
                                   datapath_usage)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

  name = "dp_comm";
  if ((r = lagopus_module_register(name,
                                   dpcomm_initialize, s_dpmptr,
                                   dpcomm_start,
                                   dpcomm_shutdown,
                                   dpcomm_stop,
                                   dpcomm_finalize,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

  name = "agent";
  if ((r = lagopus_module_register(name,
                                   agent_initialize_wrap, NULL,
                                   agent_start,
                                   agent_shutdown,
                                   agent_stop,
                                   agent_finalize_wrap,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

  name = "ofp_handler";
  if ((r = lagopus_module_register(name,
                                   ofp_handler_initialize_wrap, NULL,
                                   ofp_handler_start,
                                   ofp_handler_shutdown,
                                   ofp_handler_stop,
                                   ofp_handler_finalize_wrap,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }

#ifdef ENABLE_SNMP_MODULE
  name = "snmpmgr";
  if ((r = lagopus_module_register(name,
                                   snmpmgr_initialize_wrap, NULL,
                                   snmpmgr_start,
                                   snmpmgr_shutdown,
                                   snmpmgr_stop,
                                   snmpmgr_finalize,
                                   NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
#endif /* ENABLE_SNMP_MODULE */
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(5, "called.\n");
}


static inline void
s_final(void) {
  if (s_dpmptr != NULL) {
    dpmgr_free(s_dpmptr);
  }
}


static void
s_dtors(void) {
  s_final();

  lagopus_msg_debug(5, "called.\n");
}
