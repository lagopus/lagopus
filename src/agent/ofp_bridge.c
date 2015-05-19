/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
                  uint64_t dpid) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridge *ofpb;
  if (retptr != NULL) {
    if (*retptr == NULL) {
      ofpb = (struct ofp_bridge *)malloc(sizeof(struct ofp_bridge));
      if (ofpb == NULL) {
        res = LAGOPUS_RESULT_NO_MEMORY;
        goto done;
      }
    } else {
      ofpb = *retptr;
    }
    /*
     * create eventq, dataq, event_dataq
     */
    if ((res = lagopus_bbq_create(&(ofpb->dataq), struct eventq_data *,
                                  DATAQ_SIZE, s_eventq_freeup_proc))
        != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      ofp_bridge_destroy(ofpb);
    } else if ((res = lagopus_bbq_create(&(ofpb->eventq), struct eventq_data *,
                                         EVENTQ_SIZE, s_eventq_freeup_proc))
               != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      ofp_bridge_destroy(ofpb);
    } else if ((res = lagopus_bbq_create(&(ofpb->event_dataq),
                                         struct eventq_data *,
                                         EVENT_DATAQ_SIZE, s_eventq_freeup_proc))
               != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      ofp_bridge_destroy(ofpb);
    } else {                    /* success to create BBQs */
#ifdef OFPH_POLL_WRITING
      /* init edq_buffer */
      STAILQ_INIT(&(ofpb->edq_buffer));
#endif  /* OFPH_POLL_WRITING */
      ofpb->dpid = dpid;
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
    free(ptr);
  }
}
