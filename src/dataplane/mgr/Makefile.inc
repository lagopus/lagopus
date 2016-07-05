DPMGRSRCS = bridge.c port.c bonding.c group.c flowdb.c meter.c
DPMGRSRCS+= dp_timer.c flow_timer.c mbtree_timer.c link_timer.c thtable_timer.c
DPMGRSRCS+= desc.c queue.c dp_apis.c interface.c thread.c callback.c
ifeq (${OSDEF}, LAGOPUS_OS_LINUX)
DPMGRSRCS += sock_io.c
endif
HYBRIDSRCS = mactable.c tap_io.c updater_timer.c
HYBRIDSRCS += netlink.c rib_notifier.c rib.c route.c arp.c
PIPELINESRCS = pipeline.c
ifeq (${OSDEF}, LAGOPUS_OS_NETBSD)
DPMGRSRCS += bpf_io.c
endif
ifeq (${OSDEF}, LAGOPUS_OS_FREEBSD)
DPMGRSRCS += bpf_io.c
endif
