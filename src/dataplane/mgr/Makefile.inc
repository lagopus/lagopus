DPMGRSRCS = bridge.c port.c bonding.c group.c flowdb.c meter.c timer.c
DPMGRSRCS+= desc.c queue.c dp_apis.c interface.c thread.c
ifeq (${OSDEF}, LAGOPUS_OS_LINUX)
DPMGRSRCS += sock_io.c tap_io.c
endif
ifeq (${OSDEF}, LAGOPUS_OS_NETBSD)
DPMGRSRCS += bpf_io.c
endif
ifeq (${OSDEF}, LAGOPUS_OS_FREEBSD)
DPMGRSRCS += bpf_io.c
endif
