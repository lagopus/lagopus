#!/usr/bin/env python

import sys
import socket
import ssl
import os
import logging
import time
import random
import binascii
from contextlib import contextmanager

# lib ryu
from ryu.ofproto import ofproto_common
from ryu.ofproto import ofproto_parser
from ryu.ofproto import ofproto_v1_0
from ryu.ofproto import ofproto_v1_3
from ryu.ofproto import ofproto_protocol

# lib tool
from const import *


class TimeOut(Exception):
    pass


class OFPServer(object):

    def __init__(self, host='0.0.0.0', port=6633, is_tls=False,
                 certfile=None, keyfile=None, ca_certs=None):
        self.is_tls = is_tls
        if self.is_tls:
            self.certfile = certfile
            self.keyfile = keyfile
            self.ca_certs = ca_certs
        self.host = host
        self.port = port
        self.sock = None

    def create_sock(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        if self.is_tls:
            ssl_sock = ssl.wrap_socket(
                self.sock,
                certfile=self.certfile,
                keyfile=self.keyfile,
                ca_certs=self.ca_certs,
                cert_reqs=ssl.CERT_REQUIRED)
            self.sock = ssl_sock

        self.sock.bind((self.host, self.port))
        self.sock.listen(5)

    def accept(self, timeout):
        old_timeout = self.sock.gettimeout()
        self.sock.settimeout(timeout)

        csock, address = self.sock.accept()
        logging.info("accepted: " + str(address))

        self.sock.settimeout(old_timeout)

        return csock

    def close(self):
        self.sock.close()
        self.sock = None


class OFPDatapath(ofproto_protocol.ProtocolDesc):

    def __init__(self, sock, version=ofproto_v1_3.OFP_VERSION):
        super(OFPDatapath, self).__init__(version=version)

        self.xid = random.randint(0, self.ofproto.MAX_XID)
        # dpid
        self.id = None
        self.flow_format = ofproto_v1_0.NXFF_OPENFLOW10
        self.sock = sock
        self.data = bytearray()
        self.timeout = 0

    def close(self):
        self.sock.close()
        self.sock = None

    def recv(self, length):
        while len(self.data) <= length:
            part = self.sock.read(
                BUFSIZE) if isinstance(self.sock, ssl.SSLSocket) else self.sock.recv(BUFSIZE)
            if not part:
                raise RuntimeError("connection broken!")
            self.data += part

            # check ofp_header len.
            if len(self.data) < length:
                continue

    def recv_ofp(self):
        logging.debug("%s", binascii.hexlify(self.data))

        self.recv(ofproto_common.OFP_HEADER_SIZE)
        version, msg_type, msg_len, xid = ofproto_parser.header(self.data)

        self.recv(msg_len)

        msg = ofproto_parser.msg(self,
                                 version, msg_type, msg_len, xid, self.data[:msg_len])

        self.data = self.data[msg_len:]

        return msg

    def recvall(self, timeout):
        if timeout:
            old_timeout = self.sock.gettimeout()
            self.sock.settimeout(timeout)

        data = self.recv_ofp()

        if timeout:
            self.sock.settimeout(old_timeout)

        return data

    def tls_sendall(self, data):
        sentlen = 0
        while sentlen < len(data):
            sent = self.sock.write(data[sentlen:])
            if sent == 0:
                raise RuntimeError("connection broken!")
            sentlen += sent

    def send(self, data, timeout):
        if isinstance(self.sock, ssl.SSLSocket):
            self.tls_sendall(data)
        else:
            self.sock.sendall(data)

    def set_xid(self, msg):
        self.xid += 1
        self.xid &= self.ofproto.MAX_XID
        msg.set_xid(self.xid)
        return self.xid

    def set_send_msg(self, msg):
        if msg.xid is None:
            self.set_xid(msg)
        msg.serialize()
        return msg

    def send_msg(self, msg, timeout):
        self.send(msg.buf, timeout)

    def recv_msg(self, timeout):
        return self.recvall(timeout)
