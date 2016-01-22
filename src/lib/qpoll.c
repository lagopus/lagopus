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
#include "qmuxer_types.h"
#include "qmuxer_internal.h"





static inline lagopus_result_t
s_poll_initialize(lagopus_qmuxer_poll_t mp,
                  lagopus_bbq_t bbq,
                  lagopus_qmuxer_poll_event_t type) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mp != NULL &&
      bbq != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {
    (void)memset((void *)mp, 0, sizeof(*mp));
    mp->m_bbq = bbq;
    mp->m_type = type;
    mp->m_q_size = 0;
    mp->m_q_rem_capacity = 0;

    ret = LAGOPUS_RESULT_OK;

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_poll_destroy(lagopus_qmuxer_poll_t mp) {
  free((void *)mp);
}





lagopus_result_t
lagopus_qmuxer_poll_create(lagopus_qmuxer_poll_t *mpptr,
                           lagopus_bbq_t bbq,
                           lagopus_qmuxer_poll_event_t type) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL) {
    *mpptr = (lagopus_qmuxer_poll_t)malloc(sizeof(**mpptr));
    if (*mpptr == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      goto done;
    }
    ret = s_poll_initialize(*mpptr, bbq, type);
    if (ret != LAGOPUS_RESULT_OK) {
      s_poll_destroy(*mpptr);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
lagopus_qmuxer_poll_destroy(lagopus_qmuxer_poll_t *mpptr) {
  if (mpptr != NULL) {
    s_poll_destroy(*mpptr);
  }
}


lagopus_result_t
lagopus_qmuxer_poll_reset(lagopus_qmuxer_poll_t *mpptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_q_size = 0;
    (*mpptr)->m_q_rem_capacity = 0;

    ret = LAGOPUS_RESULT_OK;

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_qmuxer_poll_set_queue(lagopus_qmuxer_poll_t *mpptr,
                              lagopus_bbq_t bbq) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_bbq = bbq;
    if (bbq != NULL) {
      bool tmp = false;
      ret = lagopus_bbq_is_operational(&bbq, &tmp);
      if (ret == LAGOPUS_RESULT_OK && tmp == true) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        /*
         * rather LAGOPUS_RESULT_INVALID_ARGS ?
         */
        ret = LAGOPUS_RESULT_NOT_OPERATIONAL;
      }
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_qmuxer_poll_get_queue(lagopus_qmuxer_poll_t *mpptr,
                              lagopus_bbq_t *bbqptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      bbqptr != NULL) {
    *bbqptr = (*mpptr)->m_bbq;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_qmuxer_poll_set_type(lagopus_qmuxer_poll_t *mpptr,
                             lagopus_qmuxer_poll_event_t type) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {

    if ((*mpptr)->m_bbq != NULL) {
      (*mpptr)->m_type = type;
    } else {
      (*mpptr)->m_type = LAGOPUS_QMUXER_POLL_UNKNOWN;
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_qmuxer_poll_size(lagopus_qmuxer_poll_t *mpptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_size;
    } else {
      ret = 0;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


lagopus_result_t
lagopus_qmuxer_poll_remaining_capacity(lagopus_qmuxer_poll_t *mpptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_rem_capacity;
    } else {
      ret = 0;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

