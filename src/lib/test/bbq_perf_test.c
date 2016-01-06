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

#define OUTPUT stdout
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define get_time_stamp(tp) clock_gettime(CLOCK_REALTIME, (tp))
#define SEC  1000000000
#define MSEC 1000000
#define USEC 1000


typedef struct entry {
  int num;
} entry;

typedef LAGOPUS_BOUND_BLOCK_Q_DECL(bbq, entry *) bbq;
bbq bbQ;

static lagopus_result_t
s_put(bbq *bbQptr, entry *put, lagopus_chrono_t timed_wait, double *rettime) {
  struct timespec start, end;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  get_time_stamp(&start);
  ret = lagopus_bbq_put(bbQptr, &put, entry *, timed_wait);
  get_time_stamp(&end);
  *rettime = (double)(end.tv_sec - start.tv_sec)
             + (double)(end.tv_nsec - start.tv_nsec)/1000000000;
  return ret;
}

static lagopus_result_t
s_get(bbq *bbQptr, entry **get, lagopus_chrono_t timed_wait, double *rettime) {
  struct timespec start, end;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  get_time_stamp(&start);
  ret = lagopus_bbq_get(bbQptr, get, entry *, timed_wait);
  get_time_stamp(&end);
  *rettime = (double)(end.tv_sec - start.tv_sec)
             + (double)(end.tv_nsec - start.tv_nsec)/1000000000;
  return ret;
}

void
setUp(void) {
}

void
tearDown(void) {
  lagopus_bbq_shutdown(&bbQ, true);
  lagopus_bbq_destroy(&bbQ, true);
}



/* thread value */
struct thread_value {
  bbq *bbQ;
  lagopus_chrono_t put_timed_wait;
  lagopus_chrono_t get_timed_wait;
  pthread_barrier_t barrier;
  int putget_loop;
};

/* thread value */
struct thread_return {
  int ok_cnt;
  int timeout_cnt;
  int not_op_cnt;
  int error_cnt;
  double ok_sum;
  double ok_min;
  double ok_max;
  double timeout_sum;
  double timeout_min;
  double timeout_max;
};


static inline void
s_init_t_ret(struct thread_return *t_ret) {
  if (t_ret != NULL) {
    t_ret->ok_cnt = 0;
    t_ret->timeout_cnt = 0;
    t_ret->not_op_cnt = 0;
    t_ret->error_cnt = 0;
    t_ret->ok_sum = 0;
    t_ret->ok_min = 1000000.0;
    t_ret->ok_max = 0;
    t_ret->timeout_sum = 0;
    t_ret->timeout_min = 1000000.0;
    t_ret->timeout_max = 0;
  }
}

static void
s_print_t_ret(struct thread_return *t_ret) {
  lagopus_msg_debug(1, "  ok      %6d ", t_ret->ok_cnt);
  if (t_ret->ok_cnt != 0) {
    lagopus_msg_debug(1, "min: %.10f max: %.10f avg: %.10f\n",
                      t_ret->ok_min, t_ret->ok_max,
                      t_ret->ok_sum/t_ret->ok_cnt);
  } else {
    lagopus_msg_debug(1, "min: %.10f max: %.10f avg: %.10f\n",
                      0.0, 0.0, 0.0);
  }
  lagopus_msg_debug(1, "  timeout %6d ", t_ret->timeout_cnt);
  if (t_ret->timeout_cnt != 0) {
    lagopus_msg_debug(1, "min: %.10f max: %.10f avg: %.10f\n",
                      t_ret->timeout_min, t_ret->timeout_max,
                      t_ret->timeout_sum/t_ret->timeout_cnt);
  } else {
    lagopus_msg_debug(1, "min: %.10f max: %.10f avg: %.10f\n",
                      0.0, 0.0, 0.0);
  }
  lagopus_msg_debug(1, "  error   %6d\n", t_ret->error_cnt);
  lagopus_msg_debug(1, "  not_ope %6d\n", t_ret->not_op_cnt);
}

static void
s_t_ret_add(struct thread_return *a, struct thread_return *b) {
  a->ok_cnt += b->ok_cnt;
  a->ok_sum += b->ok_sum;
  a->ok_min = min(a->ok_min, b->ok_min);
  a->ok_max = max(a->ok_max, b->ok_max);
  a->timeout_cnt += b->timeout_cnt;
  a->timeout_sum += b->timeout_sum;
  a->timeout_min = min(a->timeout_min, b->timeout_min);
  a->timeout_max = max(a->timeout_max, b->timeout_max);
  a->error_cnt += b->error_cnt;
  a->not_op_cnt += b->not_op_cnt;
}

static void *
s_run_get(void *arg) {
  lagopus_result_t ret;
  int i;
  struct thread_value *tv = (struct thread_value *)arg;
  entry *get;
  double sum;
  struct thread_return *t_ret;
  t_ret = (struct thread_return *)malloc(sizeof(struct thread_return));
  if (t_ret != NULL) {
    s_init_t_ret(t_ret);

    pthread_barrier_wait(&(tv->barrier));
    for (i = 0; i < tv->putget_loop; i++) {
      ret = s_get(tv->bbQ, &get, tv->get_timed_wait, &sum);
      switch (ret) {
        case LAGOPUS_RESULT_OK:
          t_ret->ok_cnt++;
          t_ret->ok_sum += sum;
          t_ret->ok_min = min(t_ret->ok_min, sum);
          t_ret->ok_max = max(t_ret->ok_max, sum);
          break;
        case LAGOPUS_RESULT_TIMEDOUT:
          t_ret->timeout_cnt++;
          t_ret->timeout_sum += sum;
          t_ret->timeout_min = min(t_ret->timeout_min, sum);
          t_ret->timeout_max = max(t_ret->timeout_max, sum);
          break;
        case LAGOPUS_RESULT_NOT_OPERATIONAL:
          t_ret->not_op_cnt++;
          break;
        default:
          t_ret->error_cnt++;
          break;
      }
    }
  }
  pthread_exit(t_ret);
}

static void *
s_run_put(void *arg) {
  lagopus_result_t ret;
  int i;
  struct thread_value *tv = (struct thread_value *)arg;
  double sum;
  struct thread_return *t_ret;
  t_ret = (struct thread_return *)malloc(sizeof(struct thread_return));
  if (t_ret != NULL) {
    s_init_t_ret(t_ret);

    pthread_barrier_wait(&(tv->barrier));
    for (i = 0; i < tv->putget_loop; i++) {
      ret = s_put(tv->bbQ, NULL, tv->put_timed_wait, &sum);
      switch (ret) {
        case LAGOPUS_RESULT_OK:
          t_ret->ok_cnt++;
          t_ret->ok_sum += sum;
          t_ret->ok_min = min(t_ret->ok_min, sum);
          t_ret->ok_max = max(t_ret->ok_max, sum);
          break;
        case LAGOPUS_RESULT_TIMEDOUT:
          t_ret->timeout_cnt++;
          t_ret->timeout_sum += sum;
          t_ret->timeout_min = min(t_ret->timeout_min, sum);
          t_ret->timeout_max = max(t_ret->timeout_max, sum);
          break;
        case LAGOPUS_RESULT_NOT_OPERATIONAL:
          t_ret->not_op_cnt++;
          break;
        default:
          t_ret->error_cnt++;
          break;
      }
    }
  }
  pthread_exit(t_ret);
}


static void
s_gen_test(int n_entry,
           int put_thread_num,
           int get_thread_num,
           int n_loops,
           lagopus_chrono_t put_timed_wait,
           lagopus_chrono_t get_timed_wait) {
  lagopus_result_t ret;
  pthread_t *put_threads, *get_threads;
  struct thread_value t_val;
  int i;
  struct thread_return g_result, p_result, *t_ret;

  put_threads = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)put_thread_num);
  get_threads = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)get_thread_num);

  if (put_threads != NULL && get_threads != NULL) {
    s_init_t_ret(&g_result);
    s_init_t_ret(&p_result);

    /* init bbQ */
    ret = lagopus_bbq_create(&bbQ, entry *, n_entry, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "allocate error\n");

    /* set thread value */
    t_val.bbQ = &bbQ;
    t_val.putget_loop = n_loops;
    t_val.put_timed_wait = put_timed_wait;
    t_val.get_timed_wait = get_timed_wait;
    pthread_barrier_init(&(t_val.barrier), NULL,
                         (unsigned int)(put_thread_num + get_thread_num));
    /* thread start */
    for (i = 0; i < put_thread_num; i++) {
      pthread_create(&(put_threads[i]), NULL, s_run_put, &t_val);
    }
    for (i = 0; i < get_thread_num; i++) {
      pthread_create(&(get_threads[i]), NULL, s_run_get, &t_val);
    }
    /* thread join */
    for (i = 0; i < put_thread_num; i++) {
      pthread_join(put_threads[i], (void **)&t_ret);
      s_t_ret_add(&p_result, t_ret);
      free(t_ret);
    }
    for (i = 0; i < get_thread_num; i++) {
      pthread_join(get_threads[i], (void **)&t_ret);
      s_t_ret_add(&g_result, t_ret);
      free(t_ret);
    }
    /* print results */
    lagopus_msg_debug(1, "\n--------------------\n");
    lagopus_msg_debug(1, "m_value[N]: %d\n",  n_entry);
    lagopus_msg_debug(1, "PUT: (%d loop * %d thread) TIMED_WAIT = %f\n",
                      n_loops, put_thread_num, (double)put_timed_wait/1000000000);
    s_print_t_ret(&p_result);
    lagopus_msg_debug(1, "GET: (%d loop * %d thread) TIMED_WAIT = %f\n",
                      n_loops, get_thread_num, (double)get_timed_wait/1000000000);
    s_print_t_ret(&g_result);
    /* free */
    free(put_threads);
    free(get_threads);
  } else {
    free(put_threads);
    free(get_threads);
    TEST_FAIL_MESSAGE("allocate error\n");
  }
}

void
test_bbq_multithread_10(void) {
  s_gen_test(10, 10, 10, 100, 1 * SEC, 1 * SEC);
}

void
test_bbq_multithread_10_2(void) {
  s_gen_test(10, 10, 10, 100, 1 * MSEC, 1 * MSEC);
}

void
test_bbq_multithread_100(void) {
  s_gen_test(10, 100, 100, 100, 1 * USEC, 1 * USEC);
}

/* too heavy! */
/*
void
test_bbq_multithread_1000(void) {
  s_gen_test(100, 1000, 1000, 100, 1 * USEC, 1 * USEC);
}

void
test_bbq_multithread_10000(void) {
  s_gen_test(100, 10000, 10000, 100, 10000, 10000);
}
*/
