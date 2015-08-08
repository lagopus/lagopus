/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

/**
 *      @file   pcap.c
 *      @brief  Packet capture routines
 */

#include <inttypes.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <pcap/pcap.h>

#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "pktbuf.h"
#include "packet.h"
#include "pcap.h"

#undef PCAP_DEBUG
#ifdef PCAP_DEBUG
#define DPRINT(...) printf(__VA_ARGS__)
#else
#define DPRINT(...)
#endif

/**
 * data structure for each capture thread.
 */
struct pcap_arg {
  pcap_t *pcap;
  pcap_dumper_t *dumper;
  lagopus_bbq_t queue;
};

/**
 * packet capture thread.
 *
 * pick packet data from input queue, and output to capture file.
 */
static void *
pcap_thread(void *arg) {
  struct pcap_arg *pcap_arg = arg;
  pcap_dumper_t *dumper;
  OS_MBUF *mbuf;

  dumper = pcap_arg->dumper;

  for (;;) {
    lagopus_result_t rv;
    struct pcap_pkthdr pkthdr;

    /* read blocking queue */
    rv = lagopus_bbq_get(&pcap_arg->queue, &mbuf, OS_MBUF *, -1);
    if (rv == LAGOPUS_RESULT_OK) {
      /* output file */
      DPRINT("lagopus_bbq_get success. m=%p\n", mbuf);
      /* timestamp should be set by dataplane? */
      gettimeofday(&pkthdr.ts, NULL);
      pkthdr.caplen = (uint32_t)OS_M_PKTLEN(mbuf);
      pkthdr.len = (uint32_t)OS_M_PKTLEN(mbuf);
      pcap_dump((u_char *)dumper, &pkthdr, OS_MTOD(mbuf, u_char *));
      OS_M_FREE(mbuf);
    } else if (rv != LAGOPUS_RESULT_TIMEDOUT) {
      /* error detected. */
      DPRINT("lagopus_bbq_get error %d\n", rv);
    } else {
      DPRINT("lagopus_bbq_get timeout\n");
    }
  }
  return NULL;
}

lagopus_result_t
lagopus_pcap_init(struct port *port, const char *fname) {
  struct pcap_arg *pcap_arg;
  pthread_t tid;

  pcap_arg = calloc(1, sizeof(struct pcap_arg));
  if (pcap_arg == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  lagopus_bbq_create(&port->pcap_queue, OS_MBUF *, 1000, NULL);
  pcap_arg->pcap = pcap_open_dead(DLT_EN10MB, 1500);
  pcap_arg->dumper = pcap_dump_open(pcap_arg->pcap, fname);
  if (pcap_arg->dumper == NULL) {
    pcap_close(pcap_arg->pcap);
    free(pcap_arg);
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  pcap_arg->queue = port->pcap_queue;
  pthread_create(&tid, NULL, pcap_thread, pcap_arg);

  return LAGOPUS_RESULT_OK;
}

void
lagopus_pcap_enqueue(struct port *port, struct lagopus_packet *pkt) {
  OS_MBUF *m = pkt->mbuf;
  DPRINT("%s: enqueue port: %d, data: %p\n",
         __func__, port->ofp_port.port_no, m);
  OS_M_ADDREF(m);
  lagopus_bbq_put(&port->pcap_queue, &m, OS_MBUF *, 1000);
}
