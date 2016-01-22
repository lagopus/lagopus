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

#include "unity.h"
#include "lagopus_apis.h"

#define N_ENTRY 100
#define TIMED_WAIT 10000000LL
#define N_LOOP 100

typedef struct entry {
  int num;
} entry;

typedef LAGOPUS_BOUND_BLOCK_Q_DECL(bbq, entry *) bbq;
typedef LAGOPUS_BOUND_BLOCK_Q_DECL(uint16_bbq, uint16_t) uint16_bbq;
typedef LAGOPUS_BOUND_BLOCK_Q_DECL(uint32_bbq, uint32_t) uint32_bbq;
typedef LAGOPUS_BOUND_BLOCK_Q_DECL(uint64_bbq, uint64_t) uint64_bbq;
typedef LAGOPUS_BOUND_BLOCK_Q_DECL(longdouble_bbq, long double) longdouble_bbq;
typedef LAGOPUS_BOUND_BLOCK_Q_DECL(char_bbq, char) char_bbq;


bbq bbQ;

void setUp(void);
void tearDown(void);

int free_cnt = 0;


static void
s_freeup(void **arg) {
  if (arg != NULL) {
    entry *eptr = (entry *)*arg;
    if (eptr != NULL) {
      free_cnt++;
    }
    free(eptr);
  }
}


static lagopus_result_t
s_put(bbq *qptr, entry *put) {
  entry *pptr = put;
  return lagopus_bbq_put(qptr, &pptr, entry *, TIMED_WAIT);
}

static lagopus_result_t
s_get(bbq *qptr, entry **get) {
  return lagopus_bbq_get(qptr, get, entry *, TIMED_WAIT);
}

static lagopus_result_t
s_peek(bbq *qptr, entry **peek) {
  return lagopus_bbq_peek(qptr, peek, entry *, TIMED_WAIT);
}

void
setUp(void) {
  lagopus_result_t ret;
  ret = lagopus_bbq_create(&bbQ, entry *, N_ENTRY, s_freeup);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    exit(1);
  }
}

void
tearDown(void) {
  lagopus_bbq_shutdown(&bbQ, false);
  lagopus_bbq_destroy(&bbQ, false);
}



void
test_bbq_creation(void) {
  lagopus_result_t ret;
  bbq bbqueue;

  ret = lagopus_bbq_create(&bbqueue, entry *, N_ENTRY, NULL);
  if (ret != LAGOPUS_RESULT_OK) {
    TEST_FAIL_MESSAGE("creation failed");
    lagopus_perror(ret);
    goto shutdown;
  }

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create bbq");

shutdown:
  lagopus_bbq_shutdown(&bbqueue, false);
  lagopus_bbq_destroy(&bbqueue, false);
}

void
test_bbq_creation_invalid_argument(void) {
  lagopus_result_t ret;
  bbq bbqueue;
  ret = lagopus_bbq_create(NULL, entry *, N_ENTRY, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret, "NULL bbq");
  ret = lagopus_bbq_create(&bbqueue, entry *, -1, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret, "-1 entry");
  ret = lagopus_bbq_create(&bbqueue, entry *, 0, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret, "0 entry");
}


void
test_bbq_put_get_invalid_argument(void) {
  lagopus_result_t ret;
  entry *get, *null_ptr = NULL;

  ret = lagopus_bbq_put(NULL, &(null_ptr), entry *, TIMED_WAIT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret, "put - NULL bbq");
  ret = lagopus_bbq_put(&bbQ, NULL, entry *, TIMED_WAIT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "put - NULL val_ptr");

  ret = lagopus_bbq_get(NULL, &get, entry *, TIMED_WAIT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret, "get - NULL bbq");
  ret = lagopus_bbq_get(&bbQ, NULL, entry *, TIMED_WAIT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "get - NULL val_ptr");

  ret = lagopus_bbq_peek(NULL, &get, entry *, TIMED_WAIT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret, "peek - NULL bbq");
  ret = lagopus_bbq_peek(&bbQ, NULL, entry *, TIMED_WAIT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "peek - NULL val_ptr");
}

void
test_bbq_put_get_ptr(void) {
  lagopus_result_t ret;
  int i, j;
  entry entries[N_ENTRY], *get;
  for (i = 0; i < N_ENTRY; i++) {
    entries[i].num = i;
  }

  for (j = 0; j < N_LOOP; j++) {
    for (i = 0; i < N_ENTRY; i++) {
      ret = s_put(&bbQ, &(entries[i]));
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
      }
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put entry");
    }
    for (i = 0; i < N_ENTRY; i++) {
      ret = s_peek(&bbQ, &get);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek entry");
      TEST_ASSERT_EQUAL_MESSAGE(&(entries[i]), get, "peek entry - check ptr");
      TEST_ASSERT_EQUAL_MESSAGE(entries[i].num, get->num,
                                "peek entry - check value");
      ret = s_get(&bbQ, &get);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get entry");
      TEST_ASSERT_EQUAL_MESSAGE(&(entries[i]), get, "get entry - check ptr");
      TEST_ASSERT_EQUAL_MESSAGE(entries[i].num, get->num, "get entry - check value");
    }
  }
}

void
test_bbq_put_get_null(void) {
  lagopus_result_t ret;
  int i, j;
  entry *get;
  for (j = 0; j < N_LOOP; j++) {
    for (i = 0; i < N_ENTRY; i++) {
      ret = s_put(&bbQ, NULL);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put NULL");
    }
    for (i = 0; i < N_ENTRY; i++) {
      ret = s_peek(&bbQ, &get);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek NULL");
      TEST_ASSERT_EQUAL_MESSAGE(NULL, get, "peek NULL - check value");
      ret = s_get(&bbQ, &get);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get NULL");
      TEST_ASSERT_EQUAL_MESSAGE(NULL, get, "get NULL - check value");
    }
  }
}

void
test_bbq_put_get_8bit(void) {
  lagopus_result_t ret;
  int i, j;
  char get;
  char_bbq cbbq;

  ret = lagopus_bbq_create(&cbbq, char, N_ENTRY, NULL);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto shutdown;
  }

  for (j = 0; j < N_LOOP; j++) {
    for (i = 0; i < N_ENTRY; i++) {
      char c = (char)('a' + i);
      ret = lagopus_bbq_put((&cbbq), &c, char, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put entry");
    }
    for (i = 0; i < N_ENTRY; i++) {
      char c = (char)('a' + i);
      ret = lagopus_bbq_peek((&cbbq), &get, char, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek entry");
      TEST_ASSERT_EQUAL_MESSAGE(c, get, "peek entry - check value");
      ret = lagopus_bbq_get((&cbbq), &get, char, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get entry");
      TEST_ASSERT_EQUAL_MESSAGE(c, get, "get entry - check value");
    }
  }

shutdown:
  lagopus_bbq_shutdown(&cbbq, false);
  lagopus_bbq_destroy(&cbbq, false);
}

void
test_bbq_put_get_16bit(void) {
  lagopus_result_t ret;
  uint16_t i, j, get;
  uint16_bbq ubbq;

  ret = lagopus_bbq_create(&ubbq, uint16_t, N_ENTRY, NULL);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto shutdown;
  }

  for (j = 0; j < N_LOOP; j++) {
    for (i = 0; i < N_ENTRY; i++) {
      ret = lagopus_bbq_put((&ubbq), (uint16_t *)&i, uint16_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put entry");
    }
    for (i = 0; i < N_ENTRY; i++) {
      ret = lagopus_bbq_peek((&ubbq), (&get), uint16_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek entry");
      TEST_ASSERT_EQUAL_UINT16_MESSAGE(i, get, "peek entry - check value");
      ret = lagopus_bbq_get((&ubbq), (&get), uint16_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get entry");
      TEST_ASSERT_EQUAL_UINT16_MESSAGE(i, get, "get entry - check value");
    }
  }

shutdown:
  lagopus_bbq_shutdown(&ubbq, false);
  lagopus_bbq_destroy(&ubbq, false);
}

void
test_bbq_put_get_32bit(void) {
  lagopus_result_t ret;
  uint32_t i, j, get;
  uint32_bbq ubbq;

  ret = lagopus_bbq_create(&ubbq, uint32_t, N_ENTRY, NULL);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto shutdown;
  }

  for (j = 0; j < N_LOOP; j++) {
    for (i = 0; i < N_ENTRY; i++) {
      ret = lagopus_bbq_put((&ubbq), (uint32_t *)&i, uint32_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put entry");
    }
    for (i = 0; i < N_ENTRY; i++) {
      ret = lagopus_bbq_peek((&ubbq), (&get), uint32_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek entry");
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(i, get, "peek entry - check value");
      ret = lagopus_bbq_get((&ubbq), (&get), uint32_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get entry");
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(i, get, "get entry - check value");
    }
  }

shutdown:
  lagopus_bbq_shutdown(&ubbq, true);
  lagopus_bbq_destroy(&ubbq, true);
}

void
test_bbq_put_get_64bit(void) {
  lagopus_result_t ret;
  uint64_t i, j, get;
  uint64_bbq ubbq;

  ret = lagopus_bbq_create(&ubbq, uint64_t, N_ENTRY, NULL);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto shutdown;
  }

  for (j = 0; j < N_LOOP; j++) {
    for (i = 0; i < N_ENTRY; i++) {
      ret = lagopus_bbq_put((&ubbq), (uint64_t *)&i, uint64_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put entry");
    }
    for (i = 0; i < N_ENTRY; i++) {
      ret = lagopus_bbq_peek((&ubbq), (&get), uint64_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek entry");
      TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, get, "peek entry - check value");
      ret = lagopus_bbq_get((&ubbq), (&get), uint64_t, TIMED_WAIT);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get entry");
      TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, get, "get entry - check value");
    }
  }

shutdown:
  lagopus_bbq_shutdown(&ubbq, false);
  lagopus_bbq_destroy(&ubbq, false);
}


void
test_bbq_put_get_timedout(void) {
  lagopus_result_t ret;
  int i;
  entry *get;

  /* put ok */
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_put(&bbQ, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put-OK");
  }

  /* put timedout */
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_put(&bbQ, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_TIMEDOUT, ret, "put-TIMEDOUT");
  }

  /* peek ok */
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_peek(&bbQ, &get);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek-OK");
  }

  /* get ok */
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_get(&bbQ, &get);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get-OK");
  }

  /* get timedout */
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_get(&bbQ, &get);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_TIMEDOUT, ret, "get-TIMEDOUT");
  }

  /* peek timedout */
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_peek(&bbQ, &get);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_TIMEDOUT, ret, "peek-TIMEDOUT");
  }
}

void
test_bbq_length_invalid_argument(void) {
  bool result;
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_bbq_is_empty(NULL, &result),
                            "is_empty(NULL)");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_bbq_is_full(NULL, &result),
                            "is_full(NULL)");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_bbq_size(NULL),
                            "length(NULL)");
}


void
test_bbq_length(void) {
  lagopus_result_t ret;
  int i, j;
  entry *get;
  bool result;

  for (j = 0; j < N_LOOP; j++) {
    /* put ok */
    ret = lagopus_bbq_is_empty(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_empty");
    TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_empty");
    for (i = 0; i < N_ENTRY; i++) {
      TEST_ASSERT_EQUAL_MESSAGE(i, lagopus_bbq_size(&bbQ), "check length");
      ret = lagopus_bbq_is_full(&bbQ, &result);
      TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_full");
      TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_full");
      ret = s_put(&bbQ, NULL);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put-OK");
      ret = lagopus_bbq_is_empty(&bbQ, &result);
      TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_empty");
      TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_empty");
    }
    TEST_ASSERT_EQUAL_MESSAGE(N_ENTRY, lagopus_bbq_size(&bbQ),
                              "check length");
    ret = lagopus_bbq_is_full(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_full");
    TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_full");

    /* put timedout */
    ret = s_put(&bbQ, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_TIMEDOUT, ret, "put-TIMEDOUT");
    TEST_ASSERT_EQUAL_MESSAGE(N_ENTRY, lagopus_bbq_size(&bbQ),
                              "check length");
    ret = lagopus_bbq_is_empty(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_empty");
    TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_empty");
    ret = lagopus_bbq_is_full(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_full");
    TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_full");

    /* get ok */
    for (i = N_ENTRY; i > 0; i--) {
      ret = lagopus_bbq_is_empty(&bbQ, &result);
      TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_empty");
      TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_empty");

      TEST_ASSERT_EQUAL_MESSAGE(i, lagopus_bbq_size(&bbQ), "check length");
      ret = s_peek(&bbQ, &get);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "peek-OK");

      TEST_ASSERT_EQUAL_MESSAGE(i, lagopus_bbq_size(&bbQ), "check length");
      ret = s_get(&bbQ, &get);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "get-OK");

      ret = lagopus_bbq_is_full(&bbQ, &result);
      TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_full");
      TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_full");
    }
    TEST_ASSERT_EQUAL_MESSAGE(0, lagopus_bbq_size(&bbQ), "check length");
    ret = lagopus_bbq_is_empty(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_empty");
    TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_empty");

    /* get timedout */
    ret = s_get(&bbQ, &get);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_TIMEDOUT, ret, "get-TIMEDOUT");
    TEST_ASSERT_EQUAL_MESSAGE(0, lagopus_bbq_size(&bbQ), "check length");
    ret = lagopus_bbq_is_empty(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_empty");
    TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_empty");
    ret = lagopus_bbq_is_full(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_full");
    TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_full");
  }
}


void
test_bbq_clear(void) {
  lagopus_result_t ret;
  int i, j;
  entry *get;
  bool result;

  ret = lagopus_bbq_clear(&bbQ, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "clear");
  TEST_ASSERT_EQUAL_MESSAGE(0, lagopus_bbq_size(&bbQ), "check length");
  ret = lagopus_bbq_is_empty(&bbQ, &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_empty");
  TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_empty");
  ret = lagopus_bbq_is_full(&bbQ, &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_full");
  TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_full");

  for (j = 0; j < N_LOOP; j++) {
    for (i = 0; i < N_ENTRY; i++) {
      ret = s_put(&bbQ, NULL);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put NULL");
    }
    TEST_ASSERT_EQUAL_MESSAGE(N_ENTRY, lagopus_bbq_size(&bbQ), "check length");
    ret = lagopus_bbq_is_empty(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_empty");
    TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_empty");
    ret = lagopus_bbq_is_full(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_full");
    TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_full");

    ret = lagopus_bbq_clear(&bbQ, false);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "clear");
    TEST_ASSERT_EQUAL_MESSAGE(0, lagopus_bbq_size(&bbQ), "check length");
    ret = lagopus_bbq_is_empty(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_empty");
    TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_empty");
    ret = lagopus_bbq_is_full(&bbQ, &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_full");
    TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_full");

    ret = s_get(&bbQ, &get);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_TIMEDOUT, ret, "get-TIMEDOUT");
  }

  ret = lagopus_bbq_clear(&bbQ, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "clear");
  TEST_ASSERT_EQUAL_MESSAGE(0, lagopus_bbq_size(&bbQ), "check length");
  ret = lagopus_bbq_is_empty(&bbQ, &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_empty");
  TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_empty");
  ret = lagopus_bbq_is_full(&bbQ, &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check is_full");
  TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_full");
}


void
test_bbq_clear_invalid_args(void) {
  lagopus_result_t ret;
  ret = lagopus_bbq_clear(NULL, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret, "clear NULL");
}


void
test_bbq_clear_freeup(void) {
  lagopus_result_t ret;
  int i;
  entry put = {0};
  entry *putptr = NULL;

  /* put entry, and clean(bbq, false) ... not freeup */
  free_cnt = 0;
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_put(&bbQ, &put);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put");
  }
  ret = lagopus_bbq_clear(&bbQ, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "clear");
  TEST_ASSERT_EQUAL_MESSAGE(0, free_cnt, "check not freeup");

  /* put NULL, and clean(bbq, true) ... not freeup */
  free_cnt = 0;
  for (i = 0; i < N_ENTRY; i++) {
    ret = s_put(&bbQ, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put NULL");
  }
  ret = lagopus_bbq_clear(&bbQ, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "clear");
  TEST_ASSERT_EQUAL_MESSAGE(0, free_cnt, "check not freeup");

  /* put entry, and clean(bbq, true) ... freeup */
  free_cnt = 0;
  for (i = 0; i < N_ENTRY; i++) {
    putptr = malloc(sizeof(entry));
    TEST_ASSERT_NOT_NULL_MESSAGE(putptr, " allocate putptr");
    ret = s_put(&bbQ, putptr);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put");
  }
  ret = lagopus_bbq_clear(&bbQ, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "clear");
  TEST_ASSERT_EQUAL_MESSAGE(N_ENTRY, free_cnt, "check freeup");
}

static inline void
s_test_bbq_shutdown_freeup_sub(bool create_with_freeup,
                               bool put_entry,
                               bool shutdown_with_freeup) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bbq bbqueue;
  int i;
  entry *putptr = NULL;
  bool require_freeup = (create_with_freeup == true &&
                         put_entry == true &&
                         shutdown_with_freeup == true);
  free_cnt = 0;
  if (create_with_freeup == true) {
    ret = lagopus_bbq_create(&bbqueue, entry *, N_ENTRY, &s_freeup);
  } else {
    ret = lagopus_bbq_create(&bbqueue, entry *, N_ENTRY, NULL);
  }

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create bbqueue");
  for (i = 0; i < N_ENTRY; i++) {
    if (put_entry == true) {
      putptr = malloc(sizeof(entry));
      TEST_ASSERT_NOT_NULL_MESSAGE(putptr, " allocate putptr");
      if (putptr == NULL) {
        TEST_FAIL_MESSAGE("allocation error");
        return;
      }
      putptr->num = i;
      ret = s_put(&bbqueue, putptr);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put");
      if (require_freeup == false) {
        free(putptr);
      }
    } else {
      ret = s_put(&bbqueue, NULL);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put NULL");
    }
  }
  if (shutdown_with_freeup == true) {
    lagopus_bbq_shutdown(&bbqueue, true);
  } else {
    lagopus_bbq_shutdown(&bbqueue, false);
  }

  if (require_freeup == true) {
    TEST_ASSERT_EQUAL_MESSAGE(N_ENTRY, free_cnt, "check freeup");
  } else {
    TEST_ASSERT_EQUAL_MESSAGE(0, free_cnt, "check not freeup");
  }

  lagopus_bbq_shutdown(&bbqueue, false);
  lagopus_bbq_destroy(&bbqueue, false);
}

void
test_bbq_shutdown_freeup(void) {
  s_test_bbq_shutdown_freeup_sub(true, true, false);
  s_test_bbq_shutdown_freeup_sub(true, false, true);
  s_test_bbq_shutdown_freeup_sub(true, false, false);
  s_test_bbq_shutdown_freeup_sub(false, true, true);
  s_test_bbq_shutdown_freeup_sub(false, true, false);
  s_test_bbq_shutdown_freeup_sub(false, false, true);
  s_test_bbq_shutdown_freeup_sub(false, false, false);
  s_test_bbq_shutdown_freeup_sub(true, true, true);
}


static inline void
s_test_bbq_destroy_freeup_sub(bool create_with_freeup,
                              bool put_entry,
                              bool destroy_with_freeup) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bbq bbqueue;
  int i;
  entry *putptr = NULL;
  bool require_freeup = (create_with_freeup == true &&
                         put_entry == true &&
                         destroy_with_freeup == true);

  free_cnt = 0;
  if (create_with_freeup == true) {
    ret = lagopus_bbq_create(&bbqueue, entry *, N_ENTRY, &s_freeup);
  } else {
    ret = lagopus_bbq_create(&bbqueue, entry *, N_ENTRY, NULL);
  }
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create bbqueue");
  for (i = 0; i < N_ENTRY; i++) {
    if (put_entry == true) {
      putptr = (entry *)malloc(sizeof(entry));
      TEST_ASSERT_NOT_NULL_MESSAGE(putptr, " allocate putptr");
      if (putptr == NULL) {
        TEST_FAIL_MESSAGE("allocation error");
        return;
      }
      putptr->num = i;
      ret = s_put(&bbqueue, putptr);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put");
      if (require_freeup == false) {
        free(putptr);
      }
    } else {
      ret = s_put(&bbqueue, NULL);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "put NULL");
    }
  }
  if (destroy_with_freeup == true) {
    lagopus_bbq_destroy(&bbqueue, true);
  } else {
    lagopus_bbq_destroy(&bbqueue, false);
  }

  if (require_freeup == true) {
    TEST_ASSERT_EQUAL_MESSAGE(N_ENTRY, free_cnt, "check freeup");
  } else {
    TEST_ASSERT_EQUAL_MESSAGE(0, free_cnt, "check not freeup");
  }

  lagopus_bbq_destroy(&bbqueue, false);

  return;
}

void
test_bbq_destroy_freeup(void) {
  s_test_bbq_destroy_freeup_sub(true, true, false);
  s_test_bbq_destroy_freeup_sub(true, false, true);
  s_test_bbq_destroy_freeup_sub(true, false, false);
  s_test_bbq_destroy_freeup_sub(false, true, true);
  s_test_bbq_destroy_freeup_sub(false, true, false);
  s_test_bbq_destroy_freeup_sub(false, false, true);
  s_test_bbq_destroy_freeup_sub(false, false, false);
  s_test_bbq_destroy_freeup_sub(true, true, true);
}

/* thread value */
struct thread_value {
  bbq *bbQ;
  pthread_barrier_t barrier;
  int putget_loop;
  useconds_t put_sleep;
  useconds_t get_sleep;
};

static void *
s_run_put(void *arg) {
  lagopus_result_t ret;
  bool shutdowned = false;
  int i;
  struct thread_value *tv = (struct thread_value *)arg;
  pthread_barrier_wait(&(tv->barrier));
  for (i = 0; i < tv->putget_loop; i++) {
    ret = s_put(tv->bbQ, NULL);
    if (shutdowned) {
      TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_NOT_OPERATIONAL,
                                "put: shutdowned");
    } else {
      switch (ret) {
        case LAGOPUS_RESULT_OK:
        case LAGOPUS_RESULT_TIMEDOUT:
          break;
        case LAGOPUS_RESULT_NOT_OPERATIONAL:
          shutdowned = true;
          break;
        default:
          lagopus_perror(ret);
          TEST_FAIL_MESSAGE("put error");
          break;
      }
    }
    usleep(tv->put_sleep);
  }
  pthread_exit(NULL);
}

static void *
s_run_get(void *arg) {
  lagopus_result_t ret;
  bool shutdowned = false;
  int i;
  struct thread_value *tv = (struct thread_value *)arg;
  entry *get;
  pthread_barrier_wait(&(tv->barrier));
  for (i = 0; i < tv->putget_loop; i++) {
    ret = s_get(tv->bbQ, &get);
    if (shutdowned) {
      TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_NOT_OPERATIONAL,
                                "get: shutdowned");
    } else {
      switch (ret) {
        case LAGOPUS_RESULT_OK:
        case LAGOPUS_RESULT_TIMEDOUT:
          break;
        case LAGOPUS_RESULT_NOT_OPERATIONAL:
          shutdowned = true;
          break;
        default:
          lagopus_perror(ret);
          TEST_FAIL_MESSAGE("get error");
          break;
      }
    }
    usleep(tv->get_sleep);
  }
  pthread_exit(NULL);
}

/* TODO: assertion */
#define N_THREADS 5
void
test_bbq_multithread(void) {
  pthread_t put_threads[N_THREADS], get_threads[N_THREADS];
  struct thread_value tv;
  int i;

  tv.bbQ = &bbQ;
  tv.putget_loop = N_ENTRY * 300;
  tv.put_sleep = 100;
  tv.get_sleep = 100;
  pthread_barrier_init(&(tv.barrier), NULL, N_THREADS*2);
  /* thread start */
  for (i = 0; i < N_THREADS; i++) {
    pthread_create(&(put_threads[i]), NULL, s_run_put, &tv);
    pthread_create(&(get_threads[i]), NULL, s_run_get, &tv);
  }
  /* thread join */
  for (i = 0; i < N_THREADS; i++) {
    pthread_join(put_threads[i], NULL);
    pthread_join(get_threads[i], NULL);
  }
}

/* TODO: assertion */
void
test_bbq_safe_shutdown(void) {
  pthread_t put_thread1, put_thread2, get_thread1, get_thread2;
  lagopus_result_t ret;
  bool result;
  struct thread_value tv;

  pthread_barrier_init(&(tv.barrier), NULL, 5);
  tv.bbQ = &bbQ;
  tv.putget_loop = N_ENTRY * 3;
  tv.put_sleep = 400;
  tv.get_sleep = 400;

  ret = lagopus_bbq_is_operational(&bbQ, &result);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_operational");
  TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_operational");

  /* thread start */
  pthread_create(&put_thread1, NULL, s_run_put, &tv);
  pthread_create(&put_thread2, NULL, s_run_put, &tv);
  pthread_create(&get_thread1, NULL, s_run_get, &tv);
  pthread_create(&get_thread2, NULL, s_run_get, &tv);
  /* wait few usecs, and shutdown bbQ */
  pthread_barrier_wait(&(tv.barrier));
  usleep(2000);

  ret = lagopus_bbq_is_operational(&bbQ, &result);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_operational");
  TEST_ASSERT_EQUAL_MESSAGE(true, result, "check is_operational");

  lagopus_bbq_shutdown(&bbQ, false);
  ret = lagopus_bbq_is_operational(&bbQ, &result);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK, "check is_operational");
  TEST_ASSERT_EQUAL_MESSAGE(false, result, "check is_operational");

  /* thread join */
  pthread_join(put_thread1, NULL);
  pthread_join(put_thread2, NULL);
  pthread_join(get_thread1, NULL);
  pthread_join(get_thread2, NULL);
}
