#ifndef __LAGOPUS_CHRONO_H__
#define __LAGOPUS_CHRONO_H__





/**
 *	@file	lagopus_chrono.h
 */





lagopus_chrono_t
lagopus_chrono_now(void);


lagopus_result_t
lagopus_chrono_to_timespec(struct timespec *dstptr,
                           lagopus_chrono_t nsec);


lagopus_result_t
lagopus_chrono_to_timeval(struct timeval *dstptr,
                          lagopus_chrono_t nsec);


lagopus_result_t
lagopus_chrono_from_timespec(lagopus_chrono_t *dstptr,
                             const struct timespec *specptr);


lagopus_result_t
lagopus_chrono_from_timeval(lagopus_chrono_t *dstptr,
                            const struct timeval *valptr);


lagopus_result_t
lagopus_chrono_nanosleep(lagopus_chrono_t nsec,
                         lagopus_chrono_t *remptr);


#ifdef __GNUC__
#if defined(LAGOPUS_CPU_X86_64) || defined(LAGOPUS_CPU_I386)
static inline uint64_t
lagopus_rdtsc(void) {
  uint32_t eax, edx;
  __asm__ volatile ("rdtsc" : "=a" (eax), "=d" (edx));
  return (((uint64_t)edx) << 32) | ((uint64_t)eax);
}
#else
#warning reading TSC thingies is not supported on this platform.
static inline uint64_t
lagopus_rdtsc(void) {
  lagopus_msg_warning("reading TSC thingies is not supported on this "
                      "platform.\n");
  return 0LL;
}
#endif /* LAGOPUS_CPU_X86_64 || LAGOPUS_CPU_I386 */
#else
#warning reading TSC thingies is not supported with this compiler.
static inline uint64_t
lagopus_rdtsc(void) {
  lagopus_msg_warning("reading TSC thingies is not supported with "
                      "this compiler.\n");
  return 0LL;
}
#endif /* __GNUC__ */





#endif /* ! __LAGOPUS_CHRONO_H__ */
