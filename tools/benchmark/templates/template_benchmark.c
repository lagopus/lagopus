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

#include <pcap.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <rte_config.h>
#include <rte_mbuf.h>

#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/datastore.h"
#include "dpdk/dpdk.h"
#include "dpdk/pktbuf.h"
#include "ofproto/packet.h"
#include "datastore_apis.h"
#ifdef USE_CHANNEL /* NOTE: use channel/controller in DSL. */
#include "channel_mgr.h"
#endif


{% if include_files is defined and include_files %}
{% for include_file in include_files %}
#include "{{ include_file }}"
{% endfor %}
{% endif %}

void
lagopus_meter_init(void);

static uint64_t opt_times = 1;
static uint64_t opt_batch_size = 0; /* 0 is read all packets in pcap file. */
static const char *opt_pcap_file = NULL;
static const char *opt_dsl_file = NULL;
static datastore_interp_t interp = NULL;

static inline lagopus_result_t
start_datastore(const char *dsl) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (dsl != NULL) {
    if (interp == NULL) {
#ifdef USE_CHANNEL /* NOTE: use channel/controller in DSL. */
      const char *argv0 =
          ((IS_VALID_STRING(lagopus_get_command_name()) == true) ?
           lagopus_get_command_name() : "callout");
      const char * const argv[] = { argv0, NULL };
#endif

      if ((ret = datastore_create_interp(&interp)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = datastore_set_config_file(dsl)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = datastore_initialize(0, NULL, NULL, NULL)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = dp_api_init()) != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      lagopus_meter_init();
      flowinfo_init();
      ofp_bridgeq_mgr_initialize(NULL);

#ifdef USE_CHANNEL /* NOTE: use channel/controller in DSL. */
      (void) lagopus_mainloop_set_callout_workers_number(1);
      if ((ret = lagopus_mainloop_with_callout(1, argv, NULL, NULL,
                                               false, false, true)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      channel_mgr_initialize();
#endif

      if ((ret = datastore_start()) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = load_conf_initialize(0, NULL, NULL, NULL)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = load_conf_start()) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline void
stop_datastore(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (interp != NULL) {
    if ((ret = load_conf_shutdown(SHUTDOWN_GRACEFULLY)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      return;
    }
    if ((ret = datastore_stop()) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      return;
    }

    if ((ret = load_conf_shutdown(SHUTDOWN_GRACEFULLY)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      return;
    }

    if ((ret = datastore_shutdown(SHUTDOWN_GRACEFULLY)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      return;
    }

    load_conf_finalize();
    datastore_finalize();
    datastore_destroy_interp(&interp);
#ifdef USE_CHANNEL /* NOTE: use channel/controller in DSL. */
    channel_mgr_finalize();
#endif
    dp_api_fini();
    ofp_bridgeq_mgr_destroy();
#ifdef USE_CHANNEL /* NOTE: use channel/controller in DSL. */
    if ((ret = global_state_request_shutdown(SHUTDOWN_GRACEFULLY)) ==
        LAGOPUS_RESULT_OK) {
      lagopus_mainloop_wait_thread();
    }
#endif
  }
}

static inline struct rte_mbuf *
alloc_rte_mbuf(size_t size) {
  int fd;
  void *p;
  size_t s;

  if (size <= MAX_PACKET_SZ) {
    s = APP_DEFAULT_MBUF_SIZE;
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
  fprintf(stderr, "usage: %s [-t <MEASURING_TIMES>] [-b <BATCH_SIZE>] [-C DSL_FILE] <PCAP_FILE>\n", pname);
  fprintf(stderr, "\n");
  fprintf(stderr, "positional arguments:\n");
  fprintf(stderr, "  PCAP_FILE\t\tPacket capture file.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "optional arguments:\n");
  fprintf(stderr, "  -t MEASURING_TIMES\tNumber of executions of benchmark target func(default: 1).\n");
  fprintf(stderr, "  -b BATCH_SIZE\t\tSize of batches(default: 0).\n");
  fprintf(stderr, "               \t\t0 is read all packets in pcap file.\n");
  fprintf(stderr, "  -C DSL_FILE\t\tDSL file.\n");
}

static inline lagopus_result_t
parse_args(int argc, const char *const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int opt;

  while ((opt = getopt(argc, (char * const *) argv, "t:b:C:")) != EOF) {
    switch (opt) {
      case 't':
        ret = lagopus_str_parse_uint64(optarg, &opt_times);
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

  if (argc - optind == 1) {
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
print_statistics(lagopus_statistic_t *throughput,
                 lagopus_statistic_t *nspp) {
  fprintf(stdout, "============\n");
  print_statistic("throughput (M packets/sec):\n",
                  throughput, 1.0 / 1000.0);
  print_statistic("nsec/packet:\n",
                  nspp, 1.0 / 1000000.0);
  fprintf(stdout, "============\n");
}

static inline lagopus_result_t
measure_benchmark(struct rte_mbuf **pkts,
                  lagopus_statistic_t *throughput,
                  lagopus_statistic_t *nspp,
                  size_t count) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i, j;
  lagopus_chrono_t start_time = 0LL;
  lagopus_chrono_t end_time = 0LL;
  lagopus_chrono_t time = 0LL;
  double dthroughput, dnspp;

  for (i = 0; i < (size_t) opt_times; i++) {
{% if setup_func is defined and setup_func %}
    {{ setup_func }}();
{% endif %}

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

{% if teardown_func is defined and teardown_func %}
    {{ teardown_func }}();
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
    start_datastore(opt_dsl_file);
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
      pkts[count]->buf_addr = &pkts[count][1];
      memcpy(pkts[count]->buf_addr, p, header.caplen);
      pkts[count]->buf_len = (uint16_t) header.caplen;
      pkts[count]->data_len = (uint16_t) header.caplen;
      pkts[count]->pkt_len = header.caplen;
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

  ret = measure_benchmark(pkts, &throughput, &nspp, count);
  if (ret != LAGOPUS_RESULT_OK) {
    goto done;
  }

  print_statistics(&throughput, &nspp);

done:
  if (opt_dsl_file != NULL) {
    stop_datastore();
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
