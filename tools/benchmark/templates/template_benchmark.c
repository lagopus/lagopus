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

#include <sys/mman.h>
#include <fcntl.h>
#include <pcap.h>
#include <papi.h>

#include <rte_config.h>
#include <rte_mbuf.h>

#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofp_handler.h"
#include "dpdk/dpdk.h"
#include "dpdk/pktbuf.h"
#include "ofproto/packet.h"
#include "ofp_dpqueue_mgr.h"
#include "agent.h"


{% if include_files is defined and include_files %}
{% for include_file in include_files %}
#include "{{ include_file }}"
{% endfor %}
{% endif %}

#define IS_USED_PAPI (((opt_num_measurements) == 1) ? true : false)

static uint64_t opt_num_measurements = 1;
static uint64_t opt_batch_size = 0; /* 0 is read all packets in pcap file. */
static const char *opt_pcap_file = NULL;
static const char *opt_dsl_file = NULL;
static const char *papi_event[] = {"perf::PERF_COUNT_HW_CACHE_REFERENCES",
                                   "perf::PERF_COUNT_HW_CACHE_MISSES",
                                   "perf::PERF_COUNT_HW_CPU_CYCLES",
                                   "perf::PERF_COUNT_SW_PAGE_FAULTS"};
static size_t papi_event_num = sizeof(papi_event) / sizeof(const char *);


static lagopus_result_t
agent_initialize_wrap(int argc, const char *const argv[],
                      void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;

  return agent_initialize(NULL, retptr);
}

static void
agent_finalize_wrap(void) {
  agent_finalize();
}

static lagopus_result_t
ofp_handler_initialize_wrap(int argc, const char *const argv[],
                            void *extarg, lagopus_thread_t **retptr) {
  (void)extarg;
  (void)argc;
  (void)argv;
  (void)retptr;

  return ofp_handler_initialize(NULL, NULL);
}


static void
ofp_handler_finalize_wrap(void) {
  ofp_handler_finalize();
}

static inline lagopus_result_t
start_modules(int argc, const char *const argv[], const char *dsl) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *name;

  if (dsl != NULL) {

    name = "datastore";
    if ((ret = lagopus_module_register(name,
                                       datastore_initialize, NULL,
                                       datastore_start,
                                       datastore_shutdown,
                                       datastore_stop,
                                       datastore_finalize,
                                       NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    dp_api_init();
    name = "dp_rawsock";
    if ((ret = lagopus_module_register(name,
                                       dp_rawsock_thread_init,
                                       NULL,
                                       dp_rawsock_thread_start,
                                       dp_rawsock_thread_shutdown,
                                       dp_rawsock_thread_stop,
                                       dp_rawsock_thread_fini,
                                       NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    name = "dp_dpdk";
    if ((ret = lagopus_module_register(name,
                                       dp_dpdk_thread_init,
                                       NULL,
                                       dp_dpdk_thread_start,
                                       dp_dpdk_thread_shutdown,
                                       dp_dpdk_thread_stop,
                                       dp_dpdk_thread_fini,
                                       dp_dpdk_thread_usage)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    name = "dp_timer";
    if ((ret = lagopus_module_register(name,
                                       dp_timer_thread_init,
                                       NULL,
                                       dp_timer_thread_start,
                                       dp_timer_thread_shutdown,
                                       dp_timer_thread_stop,
                                       dp_timer_thread_fini,
                                       NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    name = "dpqueuemgr";
    if ((ret = lagopus_module_register(name,
                                       ofp_dpqueue_mgr_initialize,
                                       NULL,
                                       ofp_dpqueue_mgr_start,
                                       ofp_dpqueue_mgr_shutdown,
                                       ofp_dpqueue_mgr_stop,
                                       ofp_dpqueue_mgr_finalize,
                                       NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    name = "agent";
    if ((ret = lagopus_module_register(name,
                                       agent_initialize_wrap, NULL,
                                       agent_start,
                                       agent_shutdown,
                                       agent_stop,
                                       agent_finalize_wrap,
                                       NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    name = "ofp_handler";
    if ((ret = lagopus_module_register(name,
                                       ofp_handler_initialize_wrap, NULL,
                                       ofp_handler_start,
                                       ofp_handler_shutdown,
                                       ofp_handler_stop,
                                       ofp_handler_finalize_wrap,
                                       NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    name = "load_conf";
    if ((ret = lagopus_module_register(name,
                                       load_conf_initialize, NULL,
                                       load_conf_start,
                                       load_conf_shutdown,
                                       NULL,
                                       load_conf_finalize,
                                       NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = datastore_set_config_file(dsl)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = lagopus_mainloop_with_callout(argc, argv, NULL, NULL,
                                             false, false, true)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline void
stop_modules(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = global_state_request_shutdown(SHUTDOWN_GRACEFULLY)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    return;
  }
}

static inline lagopus_result_t
init_papi(int *EventSet) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  int rv, EventCode;

  rv = PAPI_library_init(PAPI_VER_CURRENT);
  if(rv != PAPI_VER_CURRENT) {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_perror(ret);
    fprintf(stderr, "PAPI library init error: %d.\n", rv);
    goto done;
  }

  if ((rv = PAPI_create_eventset(EventSet)) != PAPI_OK) {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_perror(ret);
    fprintf(stderr, "PAPI error: %d.\n", rv);
    goto done;
  }

  for (i = 0; i < papi_event_num; i++) {
    if ((rv = PAPI_event_name_to_code((char *) papi_event[i], &EventCode)) != PAPI_OK) {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
      lagopus_perror(ret);
      fprintf(stderr, "PAPI error(%s) : %d.\n", papi_event[i], rv);
      goto done;
    }

    if ((rv = PAPI_add_event(*EventSet, EventCode)) != PAPI_OK) {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
      lagopus_perror(ret);
      fprintf(stderr, "PAPI error(%s) : %d.\n", papi_event[i], rv);
      goto done;
    }
  }
  ret = LAGOPUS_RESULT_OK;

done:
  return ret;
}

static inline lagopus_result_t
start_papi(int *EventSet) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int rv;

  if ((rv = PAPI_start(*EventSet)) != PAPI_OK) {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_perror(ret);
    fprintf(stderr, "PAPI error: %d.\n", rv);
    goto done;
  }
  ret = LAGOPUS_RESULT_OK;

done:
  return ret;
}

static inline lagopus_result_t
stop_papi(int *EventSet,
          long long int *papi_values) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int rv;

  if ((rv = PAPI_stop(*EventSet, papi_values)) != PAPI_OK) {
    lagopus_perror(ret);
    fprintf(stderr, "PAPI error: %d.\n", rv);
    goto done;
  }

  if ((rv = PAPI_reset(*EventSet)) != PAPI_OK) {
    lagopus_perror(ret);
    fprintf(stderr, "PAPI error: %d.\n", rv);
    goto done;
  }
  ret = LAGOPUS_RESULT_OK;

done:
  return ret;
}

static inline struct rte_mbuf *
alloc_rte_mbuf(size_t size) {
  int fd;
  void *p;
  size_t s;

  if (size <= MAX_PACKET_SZ) {
    s = sizeof(struct rte_mbuf) +
        DP_MBUF_ROUNDUP(sizeof(struct lagopus_packet),
                        RTE_MBUF_PRIV_ALIGN) +
        RTE_PKTMBUF_HEADROOM + MAX_PACKET_SZ;
    fd = open("/dev/zero", O_RDONLY);
    if(fd >= 0) {
      p = mmap(NULL, s, PROT_READ | PROT_WRITE , MAP_PRIVATE, fd, 0);
      if (p == MAP_FAILED) {
        p = NULL;
        goto done;
      }
      mlock(p, size);

   done:
      close(fd);
    } else {
      p = NULL;
    }
  } else {
    lagopus_perror(LAGOPUS_RESULT_TOO_LONG);
    p = NULL;
  }

  return (struct rte_mbuf *) p;
}

static inline void
free_rte_mbuf(struct rte_mbuf *p, size_t size) {
  if (p != NULL) {
    size += sizeof(struct rte_mbuf);
    munlock((void *) p, size);
    munmap((void *)p, size);
  }
}

static inline void
print_usage(const char *pname) {
  fprintf(stderr, "usage: %s [-n <NUM_MEASUREMENTS>] [-b <BATCH_SIZE>] "
          "[-C DSL_FILE] [-- DPDK_OPTS] [-- DP_OPTS] <PCAP_FILE>\n", pname);
  fprintf(stderr, "\n");
  fprintf(stderr, "positional arguments:\n");
  fprintf(stderr, "  PCAP_FILE\t\tPacket capture file.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "optional arguments:\n");
  fprintf(stderr, "  -n NUM_MEASUREMENTS\tNumber of executions of "
          "benchmark target func(default: 1).\n");
  fprintf(stderr, "  -b BATCH_SIZE\t\tSize of batches(default: 0).\n");
  fprintf(stderr, "               \t\t0 is read all packets in pcap file.\n");
  fprintf(stderr, "  -C DSL_FILE\t\tDSL file.\n");
  fprintf(stderr, "  DPDK_OPTS\t\tDPDK opts. "
          "It's required for use with -C.\n");
  fprintf(stderr, "  DP_OPTS\t\tDataplane opts. "
          "It's required for use with -C.\n");
}

static inline lagopus_result_t
parse_args(int argc, const char *const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int opt;

  while ((opt = getopt(argc, (char * const *) argv, "n:b:C:")) != EOF) {
    switch (opt) {
      case 'n':
        ret = lagopus_str_parse_uint64(optarg, &opt_num_measurements);
        break;
      case 'b':
        ret = lagopus_str_parse_uint64(optarg, &opt_batch_size);
        break;
      case 'C':
        opt_dsl_file = optarg;
        ret = LAGOPUS_RESULT_OK;
        break;
      default:
        ret = LAGOPUS_RESULT_INVALID_ARGS;
        goto done;
    }
  }

  if (argc != optind ) {
    opt_pcap_file = argv[optind];
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline void
print_statistic(const char *msg,
                lagopus_statistic_t *statistic,
                double scale) {
  int64_t min = 0;
  int64_t max = 0;
  double avg = 0.0;
  double dmin = 0.0;
  double dmax = 0.0;

  lagopus_statistic_min(statistic, &min);
  dmin = (double) min * scale;

  lagopus_statistic_max(statistic, &max);
  dmax = (double) max * scale;

  lagopus_statistic_average(statistic, &avg);
  avg  = avg * scale;

  fprintf(stdout, "%s", msg);
  fprintf(stdout, "  min: %f\n", dmin);
  fprintf(stdout, "  max: %f\n", dmax);
  fprintf(stdout, "  avg: %f\n", avg);
}

static inline void
print_papi(long long int *values) {
  fprintf(stdout, "cache:\n");
  fprintf(stdout, "  references: %lld\n", values[0]);
  fprintf(stdout, "  misses:     %lld\n", values[1]);
  fprintf(stdout, "  misses(%%):  %.1f\n",
          (double) values[1] / (double) values[0] * 100.0);
  fprintf(stdout, "CPU cycles : %lld\n", values[2]);
  fprintf(stdout, "page faults : %lld\n", values[3]);
}

static inline void
print_statistics(lagopus_statistic_t *throughput,
                 lagopus_statistic_t *nspp,
                 long long int *papi_values) {
  fprintf(stdout, "============\n");
  print_statistic("throughput (M packets/sec):\n",
                  throughput, 1.0 / 1000.0);
  print_statistic("nsec/packet:\n",
                  nspp, 1.0 / 1000000.0);

  if (IS_USED_PAPI == true) {
    print_papi(papi_values);
  }
  fprintf(stdout, "============\n");
}

static inline lagopus_result_t
measure_benchmark(struct rte_mbuf **pkts,
                  lagopus_statistic_t *throughput,
                  lagopus_statistic_t *nspp,
                  size_t count,
                  long long int *papi_values) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i, j;
  int EventSet = PAPI_NULL;
  lagopus_chrono_t start_time = 0LL;
  lagopus_chrono_t end_time = 0LL;
  lagopus_chrono_t time = 0LL;
  double dthroughput, dnspp;

  if (IS_USED_PAPI == true) {
    if ((ret = init_papi(&EventSet)) != LAGOPUS_RESULT_OK) {
      goto done;
    }
  }

  for (i = 0; i < (size_t) opt_num_measurements; i++) {
{% if setup_func is defined and setup_func %}
    ret = {{ setup_func }}((void *) pkts, (size_t) count);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
{% endif %}

    if (IS_USED_PAPI == true) {
      if ((ret = start_papi(&EventSet)) != LAGOPUS_RESULT_OK) {
        goto done;
      }
    }

    for (j = 0; j < count; j++) {
      rte_prefetch0(rte_pktmbuf_mtod(pkts[j],
                                     unsigned char *));
    }

    WHAT_TIME_IS_IT_NOW_IN_NSEC(start_time);

    /* call benchmark target func. */
{% if target_func is defined and target_func %}
    ret = {{ target_func }}((void *) pkts, (size_t) count);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
{% else %}
    ret = LAGOPUS_RESULT_OK;
{% endif %}

    WHAT_TIME_IS_IT_NOW_IN_NSEC(end_time);

    if (IS_USED_PAPI == true) {
      if ((ret = stop_papi(&EventSet, papi_values)) !=
          LAGOPUS_RESULT_OK) {
        goto done;
      }
    }

{% if teardown_func is defined and teardown_func %}
    ret = {{ teardown_func }}((void *) pkts, (size_t) count);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
{% endif %}

    time = end_time - start_time;
    dthroughput = (double) count / (double) time * 1000000.0;
    dnspp = (double) time / (double) count * 1000000.0;
    lagopus_statistic_record(throughput, (int64_t) (dthroughput));
    lagopus_statistic_record(nspp, (int64_t) (dnspp));
  }

done:
  return ret;
}

int
main(int argc, const char *const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  pcap_t *pcap = NULL;
  char errbuf[PCAP_ERRBUF_SIZE];
  size_t i, count;
  long long int papi_values[papi_event_num];
  const u_char *p = NULL;
  static lagopus_statistic_t throughput = NULL;
  static lagopus_statistic_t nspp = NULL;
  struct rte_mbuf **pkts = NULL;
  struct pcap_pkthdr header;

  if ((ret = parse_args(argc, argv)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    print_usage(argv[0]);
    goto done;
  }

  if ((ret = lagopus_statistic_create(&throughput, "throughput")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_statistic_create(&nspp, "nspp")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* read pcap. */
  pcap = pcap_open_offline(opt_pcap_file, errbuf);

  if (pcap == NULL) {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
    fprintf(stderr, "can't open pcap file (%s).\n", opt_pcap_file);
    goto done;
  }

  if (opt_dsl_file != NULL) {
    start_modules(argc, argv, opt_dsl_file);
  }

  count = 0;
  while ((p = pcap_next(pcap, &header)) != NULL) {
    pkts = (struct rte_mbuf **)
        realloc(pkts, sizeof(struct rte_mbuf *) * (count + 1));
    if (pkts != NULL) {
      pkts[count] = alloc_rte_mbuf(header.caplen);
      if (pkts[count] == NULL) {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        lagopus_perror(ret);
        goto done;
      }
      rte_pktmbuf_reset(pkts[count]);
      rte_mbuf_refcnt_set(pkts[count], 1);
      pkts[count]->priv_size = DP_MBUF_ROUNDUP(sizeof(struct lagopus_packet),
                                               RTE_MBUF_PRIV_ALIGN);
      pkts[count]->buf_addr = ((uint8_t *)&pkts[count][1]) + pkts[count]->priv_size;
      memcpy(pkts[count]->buf_addr, p, header.caplen);
      pkts[count]->buf_len = RTE_PKTMBUF_HEADROOM + MAX_PACKET_SZ;
      pkts[count]->data_len = (uint16_t) header.caplen;
      count++;
      if (opt_batch_size !=0 && count >= opt_batch_size) {
        break;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
      goto done;
    }
  }
  fprintf(stdout, "read %zu packets.\n", count);

  ret = measure_benchmark(pkts, &throughput, &nspp, count, papi_values);
  if (ret != LAGOPUS_RESULT_OK) {
    goto done;
  }

  print_statistics(&throughput, &nspp, papi_values);

done:
  if (opt_dsl_file != NULL) {
    stop_modules();
  }

  /* free. */
  if (pkts != NULL) {
    for (i = 0; i < count; i++) {
      free_rte_mbuf(pkts[i], pkts[i]->data_len);
    }
    free(pkts);
  }
  lagopus_statistic_destroy(&throughput);
  lagopus_statistic_destroy(&nspp);

  return (ret == LAGOPUS_RESULT_OK) ? 0 : 1;
}
