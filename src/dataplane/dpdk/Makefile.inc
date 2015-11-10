#

DATAPATHSRCS += dpdk.c dpdk_io.c worker.c config.c meter.c queue.c
DATAPATHSRCS += rte_eth_pipe.c
ifeq (${OSDEF}, LAGOPUS_OS_FREEBSD)
DATAPATHSRCS += bpf_io.c
endif
