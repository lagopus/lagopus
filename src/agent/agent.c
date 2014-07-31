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


/* OpenFlow agent. */

#include <stdio.h>
#include <stdlib.h>

#include "lagopus/dpmgr.h"
#include "lagopus/port.h"
#include "lagopus/session_tls.h"

#include "agent.h"
#include "agent_config.h"
#include "channel.h"
#include "channel_mgr.h"
#include "confsys.h"
#include "checkcert.h"

static bool running = false;
static bool is_gone = false;
static struct event_manager *em;
struct dpmgr *dpmgr;
struct channel *channel = NULL;
static lagopus_thread_t agent_thread = NULL;
static lagopus_mutex_t lock = NULL;
static pthread_once_t initialized = PTHREAD_ONCE_INIT;

static lagopus_result_t
loop(__UNUSED const lagopus_thread_t *t, __UNUSED void *arg) {
  struct event *event;

  while (running && (event = event_fetch(em)) != NULL) {
    event_call(event);
  }

  is_gone = true;

  return LAGOPUS_RESULT_OK;
}


static void
initialize_internal(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = lagopus_thread_create(&agent_thread, loop, NULL, NULL,
                              "agent", NULL);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_exit_fatal("lagopus_thread_create");
  }
  lagopus_msg_info("agent thread is created %p\n", agent_thread);

  /* Event manager allocation. */
  em = event_manager_alloc();
  if (em == NULL) {
    lagopus_exit_fatal("event manager allocate fail.");
  }

  /* Initialize channel manager. */
  channel_mgr_initialize((void *) em);

  /* initialize config tree related to agent module */
  agent_initialize_config_tree();

  /* Initialize aget check cert. */
  agent_check_certificates_initialize();

  /* Set trust point check function. */
  session_tls_certcheck_set(agent_check_certificates);

  /* Install callback. */
  config_install_callback();
  agent_install_callback();

  /* Create lagopus_mutex_create.  */
  ret = lagopus_mutex_create(&lock);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("lagopus_mutex_create");
  }
}

lagopus_result_t
agent_initialize(void *arg, lagopus_thread_t **thdptr) {
  if (agent_thread == NULL || dpmgr == NULL) {
    dpmgr = (struct dpmgr *)arg;
    lagopus_msg_info("agent_initiaize dpmgr=%p\n", dpmgr);
    pthread_once(&initialized, initialize_internal);
  } else {
    if (thdptr != NULL) {
      *thdptr = NULL;
    }
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  }

  if (thdptr != NULL) {
    *thdptr = &agent_thread;
  }

  return LAGOPUS_RESULT_OK;
}

void
agent_finalize(void) {
  bool is_valid = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_valid(&agent_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }

  agent_check_certificates_finalize();

  channel_free(channel);
  channel = NULL;
  event_manager_free(em);
  em = NULL;

  lagopus_mutex_destroy(&lock);
  lagopus_thread_destroy(&agent_thread);
  agent_thread = NULL;

  return;
}

/* Agent thread start. */
lagopus_result_t
agent_start(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_mutex_lock(&lock);
  if (running == true) {
    goto done;
  }

  running = true;
  ret = lagopus_thread_start(&agent_thread, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  lagopus_mutex_unlock(&lock);

  return ret;
}

lagopus_result_t
agent_shutdown(shutdown_grace_level_t level) {
  bool is_valid = false;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("shutdown called\n");
  ret = lagopus_thread_is_valid(&agent_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return ret;
  }

  lagopus_mutex_lock(&lock);

  running = false;

  if (level == SHUTDOWN_RIGHT_NOW) {
    event_manager_stop(em);
  }

  if (is_gone == false) {
    goto done;
  }

done:
  lagopus_mutex_unlock(&lock);

  return ret;
}

/* Agent thread stop. */
lagopus_result_t
agent_stop(void) {
  bool is_canceled = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_canceled(&agent_thread, &is_canceled);
  if (ret == LAGOPUS_RESULT_OK && is_canceled == false) {
    ret = lagopus_thread_cancel(&agent_thread);
  }

  return ret;
}
