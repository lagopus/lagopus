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

#include "lagopus/ofp_bridge.h"
#include "lagopus_apis.h"
#include "lagopus/eventq_data.h"


static void
s_eventq_freeup_proc(void **val) {
  if (val != NULL) {
    struct eventq_data *data = (struct eventq_data *)*val;
    if (data != NULL) {
      if (data->free != NULL) {
        data->free(data);
      } else {
        free(data);
      }
    }
  }
}


/**
 * create ofp_bridge
 */
lagopus_result_t
ofp_bridge_create(struct ofp_bridge **retptr,
                  uint64_t dpid,
                  const char *name,
                  datastore_bridge_info_t *info,
                  datastore_bridge_queue_info_t *q_info) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridge *ofpb;

  if (retptr != NULL && name != NULL &&
      info != NULL && q_info != NULL) {
    if (*retptr == NULL) {
      ofpb = (struct ofp_bridge *)malloc(sizeof(struct ofp_bridge));
      if (ofpb == NULL) {
        res = LAGOPUS_RESULT_NO_MEMORY;
        goto done;
      }
    } else {
      ofpb = *retptr;
    }
    ofpb->dataq = NULL;
    ofpb->eventq = NULL;
    ofpb->event_dataq = NULL;

    ofpb->name = strdup(name);
    if (ofpb->name == NULL) {
      res = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(res);
      ofp_bridge_destroy(ofpb);
      goto done;
    }

    /*
     * create eventq, dataq, event_dataq
     */
    if ((res = lagopus_bbq_create(&(ofpb->dataq), struct eventq_data *,
                                  q_info->packet_inq_size,
                                  s_eventq_freeup_proc))
        != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      ofp_bridge_destroy(ofpb);
    } else if ((res = lagopus_bbq_create(&(ofpb->eventq), struct eventq_data *,
                                         q_info->up_streamq_size,
                                         s_eventq_freeup_proc))
               != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      ofp_bridge_destroy(ofpb);
    } else if ((res = lagopus_bbq_create(&(ofpb->event_dataq),
                                         struct eventq_data *,
                                         q_info->down_streamq_size,
                                         s_eventq_freeup_proc))
               != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      ofp_bridge_destroy(ofpb);
    } else {                    /* success to create BBQs */
#ifdef OFPH_POLL_WRITING
      /* init edq_buffer */
      STAILQ_INIT(&(ofpb->edq_buffer));
#endif  /* OFPH_POLL_WRITING */
      ofpb->dpid = dpid;
      ofpb->info = *info;
      ofpb->dataq_max_batches = q_info->packet_inq_max_batches;
      ofpb->eventq_max_batches = q_info->up_streamq_max_batches;
      ofpb->event_dataq_max_batches = q_info->down_streamq_max_batches;

      *retptr = ofpb;
      res = LAGOPUS_RESULT_OK;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
done:
  return res;
}

/**
 * shutdown ofp_bridge
 */
void
ofp_bridge_shutdown(struct ofp_bridge *ptr, bool is_canceled) {
  if (ptr != NULL) {
    if (is_canceled == true) {
      lagopus_bbq_cancel_janitor(&(ptr->dataq));
      lagopus_bbq_cancel_janitor(&(ptr->eventq));
      lagopus_bbq_cancel_janitor(&(ptr->event_dataq));
    }
    lagopus_bbq_shutdown(&(ptr->dataq), true);
    lagopus_bbq_shutdown(&(ptr->eventq), true);
    lagopus_bbq_shutdown(&(ptr->event_dataq), true);
  }
}

/**
 * destroy ofp_bridge
 */
void
ofp_bridge_destroy(struct ofp_bridge *ptr) {
  if (ptr != NULL) {
    ofp_bridge_shutdown(ptr, false);
    lagopus_bbq_destroy(&(ptr->dataq), true);
    lagopus_bbq_destroy(&(ptr->eventq), true);
    lagopus_bbq_destroy(&(ptr->event_dataq), true);
    free((void *) ptr->name);
    free(ptr);
  }
}

lagopus_result_t
ofp_bridge_dataq_max_batches_set(struct ofp_bridge *ptr,
                                 uint16_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL) {
    if (ptr->dataq_max_batches != val) {
      mbar();
      ptr->dataq_max_batches = val;
      lagopus_msg_info("set dataq_max_batches: %"PRIu16".\n", val);
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_eventq_max_batches_set(struct ofp_bridge *ptr,
                                  uint16_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL) {
    if (ptr->eventq_max_batches != val) {
      mbar();
      ptr->eventq_max_batches = val;
      lagopus_msg_info("set eventq_max_batches: %"PRIu16".\n", val);
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_event_dataq_max_batches_set(struct ofp_bridge *ptr,
                                       uint16_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL) {
    if (ptr->event_dataq_max_batches != val) {
      mbar();
      lagopus_msg_info("set event_dataq_max_batches: %"PRIu16".\n", val);
      ptr->event_dataq_max_batches = val;
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_dataq_max_batches_get(struct ofp_bridge *ptr,
                                 uint16_t *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && val != NULL) {
    *val = ptr->dataq_max_batches;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_eventq_max_batches_get(struct ofp_bridge *ptr,
                                  uint16_t *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && val != NULL) {
    *val = ptr->eventq_max_batches;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_event_dataq_max_batches_get(struct ofp_bridge *ptr,
                                       uint16_t *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && val != NULL) {
    *val = ptr->event_dataq_max_batches;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_dataq_stats_get(struct ofp_bridge *ptr,
                           uint16_t *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && val != NULL) {
    ret = lagopus_bbq_size(&ptr->dataq);
    if (ret >= LAGOPUS_RESULT_OK) {
      *val = (uint16_t) ret;
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_eventq_stats_get(struct ofp_bridge *ptr,
                            uint16_t *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && val != NULL) {
    ret = lagopus_bbq_size(&ptr->eventq);
    if (ret >= LAGOPUS_RESULT_OK) {
      *val = (uint16_t) ret;
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_event_dataq_stats_get(struct ofp_bridge *ptr,
                                 uint16_t *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && val != NULL) {
    ret = lagopus_bbq_size(&ptr->event_dataq);
    if (ret >= LAGOPUS_RESULT_OK) {
      *val = (uint16_t) ret;
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_dataq_clear(struct ofp_bridge *ptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL) {
    ret = lagopus_bbq_clear(&ptr->dataq, true);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_eventq_clear(struct ofp_bridge *ptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL) {
    ret = lagopus_bbq_clear(&ptr->eventq, true);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_event_dataq_clear(struct ofp_bridge *ptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL) {
    ret = lagopus_bbq_clear(&ptr->event_dataq, true);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridge_name_get(struct ofp_bridge *ptr,
                    char **name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && name != NULL) {
    *name = strdup(ptr->name);
    if (*name == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      goto done;
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

lagopus_result_t
ofp_bridge_info_get(struct ofp_bridge *ptr,
                    datastore_bridge_info_t *info) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (ptr != NULL && info != NULL) {
    *info = ptr->info;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
