//Modified src/dataplane/ofproto/mbtree.h
#ifndef SRC_DATAPLANE_OFPROTO_LINEAR_H_
#define SRC_DATAPLANE_OFPROTO_LINEAR_H_

struct flow;
struct flow_list;
struct lagopus_packet;

struct flow *find_linear(struct lagopus_packet *pkt, struct flow_list *flow_list);

#endif 
