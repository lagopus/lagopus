/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
 *      @file   pcap.h
 *	@brief	Packet capture APIs
 */

#ifndef SRC_DATAPLANE_OFPROTO_PCAP_H_
#define SRC_DATAPLANE_OFPROTO_PCAP_H_

/**
 * Prepare packet capture function for the port.
 *
 * @param[in]   port    spcficied port for capturing packet.
 * @param[in]   fname   file name of captured data.
 *
 * @retval      LAGOPUS_RESULT_OK       success.
 *
 * If this function is successed, capture thread is created.
 * capture queue is created and make relationship with the port and the thread.
 * and capture output file is opened.
 * description for capture file format, see pcap(3p).
 */
lagopus_result_t
lagopus_pcap_init(struct port *port, const char *fname);

/**
 * Queueing packet data API.
 *
 * @param[in]   port    port structure which have capture queue.
 * @param[in]   m       packet data.
 *
 * It should be called from datapath (main thread).
 */
void
lagopus_pcap_enqueue(struct port *port, struct lagopus_packet *);

#endif /* SRC_DATAPLANE_OFPROTO_PCAP_H_ */
