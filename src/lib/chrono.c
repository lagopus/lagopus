#include "lagopus_apis.h"





lagopus_chrono_t
lagopus_chrono_now(void) {
  lagopus_chrono_t ret = 0;
  WHAT_TIME_IS_IT_NOW_IN_NSEC(ret);
  return ret;
}


lagopus_result_t
lagopus_chrono_to_timespec(struct timespec *dstptr,
                           lagopus_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TS(nsec, *dstptr);
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


lagopus_result_t
lagopus_chrono_to_timeval(struct timeval *dstptr,
                          lagopus_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TV(nsec, *dstptr);
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


lagopus_result_t
lagopus_chrono_from_timespec(lagopus_chrono_t *dstptr,
                             const struct timespec *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TS_TO_NSEC(*specptr);
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


lagopus_result_t
lagopus_chrono_from_timeval(lagopus_chrono_t *dstptr,
                            const struct timeval *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TV_TO_NSEC(*specptr);
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


lagopus_result_t
lagopus_chrono_nanosleep(lagopus_chrono_t nsec,
                         lagopus_chrono_t *remptr) {
  struct timespec t;
  struct timespec r;

  if (nsec < 0) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  NSEC_TO_TS(nsec, t);

  if (remptr == NULL) {
 retry:
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return LAGOPUS_RESULT_OK;
    } else {
      if (errno == EINTR) {
        t = r;
        goto retry;
      } else {
        return LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return LAGOPUS_RESULT_OK;
    } else {
      *remptr = TS_TO_NSEC(r);
      return LAGOPUS_RESULT_POSIX_API_ERROR;
    }
  }
}
