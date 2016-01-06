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

#ifndef SRC_INCLUDE_LAGOPUS_ETHERTYPE_H_
#define SRC_INCLUDE_LAGOPUS_ETHERTYPE_H_

/* add missing ethertype */

#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP         0x0800
#endif
#ifndef ETHERTYPE_ARP
#define ETHERTYPE_ARP        0x0806
#endif
#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN       0x8100
#endif
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6       0x86dd
#endif
#ifndef ETHERTYPE_MPLS
#define ETHERTYPE_MPLS       0x8847
#endif
#ifndef ETHERTYPE_MPLS_MCAST
#define ETHERTYPE_MPLS_MCAST 0x8848
#endif
#ifndef ETHERTYPE_PBB
#define ETHERTYPE_PBB        0x88e7
#endif

#endif /* SRC_INCLUDE_LAGOPUS_ETHERTYPE_H_ */
