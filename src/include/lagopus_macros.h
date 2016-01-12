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

#ifndef __LAGOPUS_MACROS_H__
#define __LAGOPUS_MACROS_H__





#include "lagopus_config.h"

/*
 * Architecture independent memory barrier
 */
#ifdef __GNUC__
#define mbar(...)	__sync_synchronize()
#else
#define mbar(...)	/**/
#endif /* __GNUC__ */

/*
 * Branch prediction hint
 */
#ifdef likely
#undef likely
#endif /* likely */
#ifdef unlikely
#undef unlikely
#endif /* unlikely */
#ifdef __GNUC__
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#else
#define likely(x)	(x)
#define unlikely(x)	(x)
#endif /* __GNUC__ */

/*
 * printf() type attribute
 */
#ifdef __GNUC__
#define __attr_format_printf__(x, y) \
  __attribute__ ((format(printf, x, y)))
#else
#define __attr_format_printf__(x, y) /**/
#endif /* __GNUC__ */

/*
 * Constructor attribute
 */
#ifdef __GNUC__
#define __attr_constructor__(p)	__attribute__ ((constructor(p)))
#define __attr_destructor__(p)	__attribute__ ((destructor(p)))
#else
#define __attr_constructor__(p)	/**/
#endif /* __GNUC__ */

/*
 * Macro tricks
 */
#define MACRO_STRINGIFY(x)	STRINGIFY(x)
#define STRINGIFY(x)	#x

#define MACRO_CONSTANTIFY(x, t)	CONSTANTIFY(x, t)
#define CONSTANTIFY(x, t)	x##t

#define MACRO_CONSTANTIFY_LONG(x)	MACRO_CONSTANTIFY(x, L)
#define MACRO_CONSTANTIFY_LONG_LONG(x)	MACRO_CONSTANTIFY(x, LL)

#define MACRO_CONSTANTIFY_UNSIGNED(x)		MACRO_CONSTANTIFY(x, U)
#define MACRO_CONSTANTIFY_UNSIGNED_LONG(x)	MACRO_CONSTANTIFY(x, UL)
#define MACRO_CONSTANTIFY_UNSIGNED_LONG_LONG(x)	MACRO_CONSTANTIFY(x, ULL)

/*
 * 64 bit value printing format
 */
#define PRINT_FORMAT_LONG	l
#define PRINT_FORMAT_LONG_LONG	ll

/*
 * long long is kinda obsolete since we have int64_t.
 */
#if SIZEOF_INT64_T == SIZEOF_LONG_INT
#define PRINT_FORMAT_64		PRINT_FORMAT_LONG
#elif SIZEOF_INT64_T == SIZEOF_LONG_LONG
#define PRINT_FORMAT_64		PRINT_FORMAT_LONG_LONG
#endif /* SIZEOF_INT64_T == SIZEOF_LONG_INT ... */

#define PF64(c)	\
  STRINGIFY(%) MACRO_STRINGIFY(PRINT_FORMAT_64) MACRO_STRINGIFY(c)
#define PF64S(s, c) \
  STRINGIFY(%) MACRO_STRINGIFY(s) MACRO_STRINGIFY(PRINT_FORMAT_64) \
  MACRO_STRINGIFY(c)

#ifdef HAVE_PRINT_FORMAT_FOR_SIZE_T

#define PRINT_FORMAT_SIZE_T	z

#else

#if SIZEOF_SIZE_T == SIZEOF_LONG_LONG || SIZEOF_SIZE_T == SIZEOF_INT64_T
#define PRINT_FORMAT_SIZE_T	PRINT_FORMAT_64
#elif SIZEOF_SIZE_T == SIZEOF_LONG_INT
#define PRINT_FORMAT_SIZE_T	PRINT_FORMAT_LONG
#elif SIZEOF_SIZE_T == SIZEOF_INT
#define PRINT_FORMAT_SIZE_T	d
#endif /* SIZEOF_SIZE_T == SIZEOF_LONG_LONG ... */

#endif /* HAVE_PRINT_FORMAT_FOR_SIZE_T */

#define PFSZ(c)	STRINGIFY(%) MACRO_STRINGIFY(PRINT_FORMAT_SIZE_T) \
  MACRO_STRINGIFY(c)
#define PFSZS(s, c) \
  STRINGIFY(%) MACRO_STRINGIFY(s) MACRO_STRINGIFY(PRINT_FORMAT_SIZE_T) \
  MACRO_STRINGIFY(c)

#if SIZEOF_PTHREAD_T == SIZEOF_INT64_T
#define PFTID(c) PF64(c)
#define PFTIDS(s, c) PF64S(s, c)
#elif SIZEOF_PTHREAD_T == SIZEOF_INT
#define PFTID(c) STRINGIFY(%) MACRO_STRINGIFY(c)
#define PFTIDS(s, c) STRINGIFY(%) MACRO_STRINGIFY(s) MACRO_STRINGIFY(c)
#endif /* SIZEOF_PTHREAD_T == SIZEOF_INT64_T ... */

#define IS_VALID_STRING(x)	(((x) != NULL && *(x) != '\0') ? true : false)

#define BOOL_TO_STR(a)		((a) == true) ? "true" : "false"
#define IS_BIT_SET(A, B)	((((A) & (B)) == (B)) ? true : false)

#define IS_LAGOPUS_RESULT_OK(s)	(((s) >= 0) ? true : false)

#define NSEC_TO_USEC(nsec) \
  (lagopus_chrono_t)((((nsec) / 100LL) + 5LL) / 10LL)

#ifdef MOD_IS_FASTER_THAN_MUL_SUB
#define NSEC_TO_TS(nsec, ts) \
  do {                                                              \
    (ts).tv_sec = (time_t)((nsec) / (1000LL * 1000LL * 1000LL));  \
    (ts).tv_nsec = (long)((nsec) % (1000LL * 1000LL * 1000LL));   \
  } while (0)

#define NSEC_TO_TV(nsec, tv)                                      \
  do {                                                            \
    lagopus_chrono_t __u_sec__ = NSEC_TO_USEC(nsec);              \
    (tv).tv_sec = (long)((__u_sec__) / (1000LL * 1000LL));      \
    (tv).tv_usec = (long)((__u_sec__) % (1000LL * 1000LL));     \
  } while (0)
#else
#define NSEC_TO_TS(nsec, ts) \
  do {                                                                  \
    (ts).tv_sec = (time_t)((nsec) / (1000LL * 1000LL * 1000LL));        \
    (ts).tv_nsec = (long)((nsec) -                                      \
                          (ts).tv_sec * (1000LL * 1000LL * 1000LL));    \
  } while (0)

#define NSEC_TO_TV(nsec, tv)                                        \
  do {                                                              \
    lagopus_chrono_t __u_sec__ = NSEC_TO_USEC(nsec);                \
    (tv).tv_sec = (long)((__u_sec__) / (1000LL * 1000LL));          \
    (tv).tv_usec = (long)((__u_sec__) -                             \
                          (tv).tv_sec * (1000LL * 1000LL));         \
  } while (0)
#endif /* MOD_IS_FASTER_THAN_MUL_SUB */

#define TS_TO_NSEC(ts) \
  ((lagopus_chrono_t)((ts).tv_sec) * 1000LL * 1000LL * 1000LL + \
   (lagopus_chrono_t)(ts).tv_nsec)

#define TV_TO_NSEC(tv) \
  (((lagopus_chrono_t)((tv).tv_sec) * 1000LL * 1000LL + \
    (lagopus_chrono_t)(tv).tv_usec) * 1000LL)

#define WHAT_TIME_IS_IT_NOW_IN_NSEC(ns)                    \
  do {                                                     \
    struct timespec __t_s__;                               \
    (void)clock_gettime(CLOCK_REALTIME, &__t_s__);         \
    (ns) = TS_TO_NSEC(__t_s__);                            \
  } while (0)

#ifdef LAGOPUS_BIG_ENDIAN
#ifndef htonll
#define htonll(_x64) (_x64)
#endif /* htonll */
#ifndef ntohll
#define ntohll(_x64) (_x64)
#endif /* ntohll */
#else
#ifndef htonll
#define htonll(_x64)                                     \
    ((((uint64_t) htonl((_x64) & 0xffffffffLL)) << 32) | \
     htonl((uint32_t) ((_x64) >> 32)))
#endif /* htonll */
#ifndef ntohll
#define ntohll(_x64)                                     \
    ((((uint64_t) ntohl((_x64) & 0xffffffffLL)) << 32) | \
     ntohl((uint32_t) ((_x64) >> 32)))
#endif /* ntohll */
#endif /* LAGOPUS_BIG_ENDIAN */

#ifdef __GNUC__
/* Unused argument. */
#define __UNUSED __attribute__((unused))
#endif /* __GNUC__ */

#ifdef __GNUC__
#define lagopus_atomic_update_cmp(type, addr, init, val, cmp)           \
{                                                                       \
  type __tmp__ =  __sync_fetch_and_add((addr), 0);                      \
  type __tmp2__;                                                        \
  do {                                                                  \
    if (__tmp__ == (init) || __tmp__ cmp (val)) {                       \
      __tmp2__ = __sync_val_compare_and_swap((addr), __tmp__, (val));   \
      if (likely(__tmp__ == __tmp2__)) {                                \
        break;                                                          \
      } else {                                                          \
        __tmp__ = __tmp2__;                                             \
        continue;                                                       \
      }                                                                 \
    } else {                                                            \
      break;                                                            \
    }                                                                   \
  } while (true);                                                       \
}

#define lagopus_atomic_update_min(type, addr, init, val)  \
    lagopus_atomic_update_cmp(type, addr, init, val, >)
#define lagopus_atomic_update_max(type, addr, init, val)  \
    lagopus_atomic_update_cmp(type, addr, init, val, <)
#endif /* __GNUC__ */





#endif /* ! __LAGOPUS_MACROS_H__ */
