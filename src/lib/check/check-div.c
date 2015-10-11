/* %COPYRIGHT% */


#include "lagopus_apis.h"





#define NSEC_TO_TS_MOD(nsec, ts) \
  do {                                                                  \
    (ts).tv_sec = (time_t)(nsec) / (1000LL * 1000LL * 1000LL);          \
    (ts).tv_nsec = (long)((nsec) % (1000LL * 1000LL * 1000LL));         \
  } while (0)


#define ASM_IDIVQ(devidend, divisor, quot, rem)                         \
  do {                                                                  \
    __asm__ volatile                                                    \
        ("xor %%rdx, %%rdx;"	/* clear rdx. */                        \
         "movq %2, %%rax;"	/* copy devidend into rax. */           \
         "movq %3, %%r10;"	/* copy divisor into r10. */            \
         "idiv %%r10;"		/* rdx:rax / r10. */                    \
         "movq %%rax, %0;"	/* copy the quot. */                    \
         "movq %%rdx, %1;"	/* copy the rem. */                     \
         : "=r" (quot), "=r" (rem)                                      \
         : "r" (devidend), "r" (divisor)                                \
         : "%rdx", "%rax", "%r10");                                     \
  } while (0)


#define NSEC_TO_TS_ASM(nsec, ts) \
  ASM_IDIVQ(nsec, (1000LL * 1000LL * 1000LL), ts.tv_sec, ts.tv_nsec)


int
main(int argc, const char *const argv[]) {
  size_t i;
  uint64_t zz = 0;
  struct timespec ts0;
  struct timespec ts1;
  struct timespec ts2;
  lagopus_chrono_t now;
  uint64_t start0, start1, start2, end0, end1, end2;

  WHAT_TIME_IS_IT_NOW_IN_NSEC(now);

  NSEC_TO_TS(now, ts0);
  NSEC_TO_TS_MOD(now, ts1);
  NSEC_TO_TS_ASM(now, ts2);

  fprintf(stderr, "ts0: " PF64(u) ", " PF64(u) "\n",
          ts0.tv_sec, ts0.tv_nsec);
  fprintf(stderr, "ts1: " PF64(u) ", " PF64(u) "\n",
          ts1.tv_sec, ts1.tv_nsec);
  fprintf(stderr, "ts2: " PF64(u) ", " PF64(u) "\n",
          ts2.tv_sec, ts2.tv_nsec);

  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    zz += i;
  }
  fprintf(stderr, "zz = " PF64(u) "\n", zz);

  start0 = lagopus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    NSEC_TO_TS(i, ts0);
  }
  end0 = lagopus_rdtsc();

  start1 = lagopus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    NSEC_TO_TS_MOD(i, ts1);
  }
  end1 = lagopus_rdtsc();

  start2 = lagopus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    NSEC_TO_TS_ASM(i, ts0);
  }
  end2 = lagopus_rdtsc();

  fprintf(stderr, "cur: " PF64(u) "\n", end0 - start0);
  fprintf(stderr, "mod: " PF64(u) "\n", end1 - start1);
  fprintf(stderr, "asm: " PF64(u) "\n", end2 - start2);

  return 0;
}
