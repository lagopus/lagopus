#

OFPROTOSRCS += thtable.c datapath.c crc32.c ofcache.c murmur3.c city.c mbtree.c
OFPROTOSRCS += flowinfo.c flowinfo_basic.c flowinfo_ether.c
OFPROTOSRCS += flowinfo_ipv4_proto.c flowinfo_ipv4_dst.c flowinfo_ipv4_src.c
OFPROTOSRCS += flowinfo_ipv6.c flowinfo_mpls.c flowinfo_port.c flowinfo_vlan.c
OFPROTOSRCS += flowinfo_metadata.c
OFPROTOSRCS += comm.c version.c

LDFLAGS+= -lpcap
