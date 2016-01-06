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
#include "unity.h"





#define checkOK(r)                              \
  do {                                          \
    if (r != LAGOPUS_RESULT_OK) {               \
      lagopus_perror(r);                        \
    }                                           \
    TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);    \
  } while (0)


#define checkOOR(r)                                     \
  do {                                                  \
    if (r != LAGOPUS_RESULT_OUT_OF_RANGE) {             \
      lagopus_perror(r);                                \
    }                                                   \
    TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OUT_OF_RANGE);  \
  } while (0)


#if 0
#define OUTPUT_VAL
#endif





static int64_t s_i64_max = LLONG_MAX;
static char s_i64_max_str[32];
static int64_t s_i64_min = LLONG_MIN;
static char s_i64_min_str[32];
static uint64_t	s_u64_max = ULLONG_MAX;
static char s_u64_max_str[32];

static int32_t s_i32_max = INT_MAX;
static char s_i32_max_str[32];
static int32_t s_i32_min = INT_MIN;
static char s_i32_min_str[32];
static uint32_t	s_u32_max = UINT_MAX;
static char s_u32_max_str[32];

static int16_t s_i16_max = SHRT_MAX;
static char s_i16_max_str[32];
static int16_t s_i16_min = SHRT_MIN;
static char s_i16_min_str[32];
static uint16_t	s_u16_max = USHRT_MAX;
static char s_u16_max_str[32];

static const char *const s_u128_max_str =
  "340282366920938463463374607431768211455";





void
setUp(void) {
  snprintf(s_i64_max_str, sizeof(s_i64_max_str), PF64(d), s_i64_max);
  snprintf(s_i64_min_str, sizeof(s_i64_min_str), PF64(d), s_i64_min);
  snprintf(s_u64_max_str, sizeof(s_u64_max_str), PF64(u), s_u64_max);

  snprintf(s_i32_max_str, sizeof(s_i32_max_str), "%d", s_i32_max);
  snprintf(s_i32_min_str, sizeof(s_i32_min_str), "%d", s_i32_min);
  snprintf(s_u32_max_str, sizeof(s_u32_max_str), "%u", s_u32_max);

  snprintf(s_i16_max_str, sizeof(s_i16_max_str), "%d", s_i16_max);
  snprintf(s_i16_min_str, sizeof(s_i16_min_str), "%d", s_i16_min);
  snprintf(s_u16_max_str, sizeof(s_u16_max_str), "%u", s_u16_max);
}


void
tearDown(void) {
}


static inline void
u64out(uint64_t v0, uint64_t v1) {
#ifdef OUTPUT_VAL
  fprintf(stderr, "\tv0: " PF64(u) "\n\tv1: " PF64(u) "\n",
          v0, v1);
#else
  (void)v0;
  (void)v1;
#endif /* OUTPUT_VAL */
}

static inline void
i64out(int64_t v0, int64_t v1) {
#ifdef OUTPUT_VAL
  fprintf(stderr, "\tv0: " PF64(d) "\n\tv1: " PF64(d) "\n",
          v0, v1);
#else
  (void)v0;
  (void)v1;
#endif /* OUTPUT_VAL */
}

static inline void
u32out(uint32_t v0, uint32_t v1) {
#ifdef OUTPUT_VAL
  fprintf(stderr, "\tv0: %u\n\tv1: %u\n",
          v0, v1);
#else
  (void)v0;
  (void)v1;
#endif /* OUTPUT_VAL */
}

static inline void
i32out(int32_t v0, int32_t v1) {
#ifdef OUTPUT_VAL
  fprintf(stderr, "\tv0: %d\n\tv1: %d\n",
          v0, v1);
#else
  (void)v0;
  (void)v1;
#endif /* OUTPUT_VAL */
}

static inline void
u16out(uint16_t v0, uint16_t v1) {
#ifdef OUTPUT_VAL
  fprintf(stderr, "\tv0: %u\n\tv1: %u\n",
          v0, v1);
#else
  (void)v0;
  (void)v1;
#endif /* OUTPUT_VAL */
}

static inline void
i16out(int16_t v0, int16_t v1) {
#ifdef OUTPUT_VAL
  fprintf(stderr, "\tv0: %d\n\tv1: %d\n",
          v0, v1);
#else
  (void)v0;
  (void)v1;
#endif /* OUTPUT_VAL */
}





void
test_64_range_valid(void) {
  lagopus_result_t r;

  uint64_t u64;
  int64_t i64;

  /*
   * uint64_t valid:
   */
  r = lagopus_str_parse_uint64(s_u64_max_str, &u64);
  checkOK(r);
  u64out(u64, s_u64_max);
  TEST_ASSERT_EQUAL(u64, s_u64_max);
  r = lagopus_str_parse_uint64(s_i64_max_str, &u64);
  checkOK(r);
  u64out(u64, (uint64_t)s_i64_max);
  TEST_ASSERT_EQUAL(u64, (uint64_t)s_i64_max);

  r = lagopus_str_parse_uint64(s_u32_max_str, &u64);
  checkOK(r);
  u64out(u64, (uint64_t)s_u32_max);
  TEST_ASSERT_EQUAL(u64, (uint64_t)s_u32_max);
  r = lagopus_str_parse_uint64(s_i32_max_str, &u64);
  checkOK(r);
  u64out(u64, (uint64_t)s_i32_max);
  TEST_ASSERT_EQUAL(u64, (uint64_t)s_i32_max);

  r = lagopus_str_parse_uint64(s_u16_max_str, &u64);
  checkOK(r);
  u64out(u64, (uint64_t)s_u16_max);
  TEST_ASSERT_EQUAL(u64, (uint64_t)s_u16_max);
  r = lagopus_str_parse_uint64(s_i16_max_str, &u64);
  checkOK(r);
  u64out(u64, (uint64_t)s_i16_max);
  TEST_ASSERT_EQUAL(u64, (uint64_t)s_i16_max);

  /*
   * int64_t valid:
   */
  r = lagopus_str_parse_int64(s_i64_max_str, &i64);
  checkOK(r);
  i64out(i64, s_i64_max);
  TEST_ASSERT_EQUAL(i64, s_i64_max);
  r = lagopus_str_parse_int64(s_i64_min_str, &i64);
  checkOK(r);
  i64out(i64, s_i64_min);
  TEST_ASSERT_EQUAL(i64, s_i64_min);

  r = lagopus_str_parse_int64(s_i32_max_str, &i64);
  checkOK(r);
  i64out(i64, (int64_t)s_i32_max);
  TEST_ASSERT_EQUAL(i64, (int64_t)s_i32_max);
  r = lagopus_str_parse_int64(s_i32_min_str, &i64);
  checkOK(r);
  i64out(i64, (int64_t)s_i32_min);
  TEST_ASSERT_EQUAL(i64, (int64_t)s_i32_min);

  r = lagopus_str_parse_int64(s_i16_max_str, &i64);
  checkOK(r);
  i64out(i64, (int64_t)s_i16_max);
  TEST_ASSERT_EQUAL(i64, (int64_t)s_i16_max);
  r = lagopus_str_parse_int64(s_i16_min_str, &i64);
  checkOK(r);
  i64out(i64, (int64_t)s_i16_min);
  TEST_ASSERT_EQUAL(i64, (int64_t)s_i16_min);
}


void
test_32_range_valid(void) {
  lagopus_result_t r;

  uint32_t u32;
  int32_t i32;

  /*
   * uint32_t valid:
   */
  r = lagopus_str_parse_uint32(s_u32_max_str, &u32);
  checkOK(r);
  u32out(u32, (uint32_t)s_u32_max);
  TEST_ASSERT_EQUAL(u32, (uint32_t)s_u32_max);
  r = lagopus_str_parse_uint32(s_i32_max_str, &u32);
  checkOK(r);
  u32out(u32, (uint32_t)s_i32_max);
  TEST_ASSERT_EQUAL(u32, (uint32_t)s_i32_max);

  r = lagopus_str_parse_uint32(s_u16_max_str, &u32);
  checkOK(r);
  u32out(u32, (uint32_t)s_u16_max);
  TEST_ASSERT_EQUAL(u32, (uint32_t)s_u16_max);
  r = lagopus_str_parse_uint32(s_i16_max_str, &u32);
  checkOK(r);
  u32out(u32, (uint32_t)s_i16_max);
  TEST_ASSERT_EQUAL(u32, (uint32_t)s_i16_max);

  /*
   * int32_t valid:
   */
  r = lagopus_str_parse_int32(s_i32_max_str, &i32);
  checkOK(r);
  i32out(i32, (int32_t)s_i32_max);
  TEST_ASSERT_EQUAL(i32, (int32_t)s_i32_max);
  r = lagopus_str_parse_int32(s_i32_min_str, &i32);
  checkOK(r);
  i32out(i32, (int32_t)s_i32_min);
  TEST_ASSERT_EQUAL(i32, (int32_t)s_i32_min);

  r = lagopus_str_parse_int32(s_i16_max_str, &i32);
  checkOK(r);
  i32out(i32, (int32_t)s_i16_max);
  TEST_ASSERT_EQUAL(i32, (int32_t)s_i16_max);
  r = lagopus_str_parse_int32(s_i16_min_str, &i32);
  checkOK(r);
  i32out(i32, (int32_t)s_i16_min);
  TEST_ASSERT_EQUAL(i32, (int32_t)s_i16_min);
}


void
test_16_range_valid(void) {
  lagopus_result_t r;

  uint16_t u16;
  int16_t i16;

  /*
   * uint16_t valid:
   */
  r = lagopus_str_parse_uint16(s_u16_max_str, &u16);
  checkOK(r);
  u16out(u16, (uint16_t)s_u16_max);
  TEST_ASSERT_EQUAL(u16, (uint16_t)s_u16_max);
  r = lagopus_str_parse_uint16(s_i16_max_str, &u16);
  checkOK(r);
  u16out(u16, (uint16_t)s_i16_max);
  TEST_ASSERT_EQUAL(u16, (uint16_t)s_i16_max);

  /*
   * int16_t valid:
   */
  r = lagopus_str_parse_int16(s_i16_max_str, &i16);
  checkOK(r);
  i16out(i16, (int16_t)s_i16_max);
  TEST_ASSERT_EQUAL(i16, (int16_t)s_i16_max);
  r = lagopus_str_parse_int16(s_i16_min_str, &i16);
  checkOK(r);
  i16out(i16, (int16_t)s_i16_min);
  TEST_ASSERT_EQUAL(i16, (int16_t)s_i16_min);
}





void
test_16_range_invalid(void) {
  lagopus_result_t r;

  uint16_t u16;
  int16_t i16;

  /*
   * uint16_t invalid:
   */
  r = lagopus_str_parse_uint16(s_u128_max_str, &u16);
  checkOOR(r);

  r = lagopus_str_parse_uint16(s_u64_max_str, &u16);
  checkOOR(r);
  r = lagopus_str_parse_uint16(s_i64_max_str, &u16);
  checkOOR(r);
  r = lagopus_str_parse_uint16(s_i64_min_str, &u16);
  checkOOR(r);

  r = lagopus_str_parse_uint16(s_u32_max_str, &u16);
  checkOOR(r);
  r = lagopus_str_parse_uint16(s_i32_max_str, &u16);
  checkOOR(r);
  r = lagopus_str_parse_uint16(s_i32_min_str, &u16);
  checkOOR(r);

  r = lagopus_str_parse_uint16(s_i16_min_str, &u16);
  checkOOR(r);

  /*
   * int16_t invalid:
   */
  r = lagopus_str_parse_int16(s_u128_max_str, &i16);
  checkOOR(r);

  r = lagopus_str_parse_int16(s_u64_max_str, &i16);
  checkOOR(r);
  r = lagopus_str_parse_int16(s_i64_max_str, &i16);
  checkOOR(r);
  r = lagopus_str_parse_int16(s_i64_min_str, &i16);
  checkOOR(r);

  r = lagopus_str_parse_int16(s_u32_max_str, &i16);
  checkOOR(r);
  r = lagopus_str_parse_int16(s_i32_max_str, &i16);
  checkOOR(r);
  r = lagopus_str_parse_int16(s_i32_min_str, &i16);
  checkOOR(r);

  r = lagopus_str_parse_int16(s_u16_max_str, &i16);
  checkOOR(r);
}


void
test_32_range_invalid(void) {
  lagopus_result_t r;

  uint32_t u32;
  int32_t i32;

  /*
   * uint32_t invalid:
   */
  r = lagopus_str_parse_uint32(s_u128_max_str, &u32);
  checkOOR(r);

  r = lagopus_str_parse_uint32(s_u64_max_str, &u32);
  checkOOR(r);
  r = lagopus_str_parse_uint32(s_i64_max_str, &u32);
  checkOOR(r);
  r = lagopus_str_parse_uint32(s_i64_min_str, &u32);
  checkOOR(r);

  r = lagopus_str_parse_uint32(s_i32_min_str, &u32);
  checkOOR(r);

  r = lagopus_str_parse_uint32(s_i16_min_str, &u32);
  checkOOR(r);

  /*
   * int32_t invalid:
   */
  r = lagopus_str_parse_int32(s_u128_max_str, &i32);
  checkOOR(r);

  r = lagopus_str_parse_int32(s_u64_max_str, &i32);
  checkOOR(r);
  r = lagopus_str_parse_int32(s_i64_max_str, &i32);
  checkOOR(r);
  r = lagopus_str_parse_int32(s_i64_min_str, &i32);
  checkOOR(r);

  r = lagopus_str_parse_int32(s_u32_max_str, &i32);
  checkOOR(r);
}


void
test_64_range_invalid(void) {
  lagopus_result_t r;

  uint64_t u64;
  int64_t i64;

  /*
   * uint64_t invalid:
   */
  r = lagopus_str_parse_uint64(s_u128_max_str, &u64);
  checkOOR(r);

  r = lagopus_str_parse_uint64(s_i64_min_str, &u64);
  checkOOR(r);

  r = lagopus_str_parse_uint64(s_i32_min_str, &u64);
  checkOOR(r);

  r = lagopus_str_parse_uint64(s_i16_min_str, &u64);
  checkOOR(r);

  /*
   * int64_t invalid:
   */
  r = lagopus_str_parse_int64(s_u128_max_str, &i64);
  checkOOR(r);

  r = lagopus_str_parse_int64(s_u64_max_str, &i64);
  checkOOR(r);
}





void
test_pattern(void) {
  TEST_ASSERT_EQUAL(1, 1);
}
