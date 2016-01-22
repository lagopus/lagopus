#

DATAPATHSRCS += dpdk.c dpdk_io.c worker.c config.c meter.c queue.c
DATAPATHSRCS += rte_eth_pipe.c

LDFLAGS	+= -lpcap -L$(RTE_LIBDIR) -ldpdk
