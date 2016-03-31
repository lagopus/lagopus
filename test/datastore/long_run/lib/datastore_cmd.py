#!/usr/bin/env python

import sys
import socket
import ssl
import os
import six
import select
import json
import logging
from contextlib import contextmanager

from const import *


class DataStoreCmd(object):

    def __init__(self, host="127.0.0.1", port=12345, is_tls=False,
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

        if self.is_tls:
            ssl_sock = ssl.wrap_socket(
                self.sock,
                certfile=self.certfile,
                keyfile=self.keyfile,
                ca_certs=self.ca_certs,
                cert_reqs=ssl.CERT_REQUIRED)
            self.sock = ssl_sock

    def connect(self):
        self.sock.connect((self.host, self.port))
        proto = "TLS" if self.is_tls else "TCP"
        logging.info(
            "connected: " + self.host + ":" + str(self.port) + "(" + proto + ")")

    def close(self):
        self.sock.close()
        self.sock = None

    def is_read_more(self):
        return select.select([self.sock], [], [], 0) == ([self.sock], [], [])

    def recvall(self, timeout):
        # FIXME:
        if timeout:
            old_timeout = self.sock.gettimeout()
            self.sock.settimeout(timeout)

        data = b""
        part = b""
        while True:
            part = self.sock.read(
                BUFSIZE) if self.is_tls else self.sock.recv(BUFSIZE)
            if not part:
                raise RuntimeError("connection broken!")
            data += part

            if not self.is_read_more():
                d = data
                if six.PY3:
                    d = data.decode()
                try:
                    data = json.loads(d)
                    break
                except ValueError:
                    continue

        if timeout:
            self.sock.settimeout(old_timeout)

        return data

    def tls_sendall(self, cmd):
        sentlen = 0
        while sentlen < len(cmd):
            sent = self.sock.write(cmd[sentlen:])
            if sent == 0:
                raise RuntimeError("connection broken!")
            sentlen += sent

    def send_cmd(self, cmd, timeout):
        cmd += "\n"
        c = cmd
        if six.PY3:
            c = cmd.encode()
        if self.is_tls:
            self.tls_sendall(c)
        else:
            self.sock.sendall(c)
        return self.recvall(timeout)


@contextmanager
def open_ds_cmd(**kwds):
    try:
        dsc = DataStoreCmd(**kwds)
        dsc.create_sock()
        dsc.connect()
        yield dsc
    finally:
        if dsc.sock:
            dsc.close()

if __name__ == "__main__":
    # tests
    # precondition: start lagopus.
    with open_ds_cmd() as dsc:
        six.print_(dsc.send_cmd("channel cahnnel01 create", 30))
        six.print_(dsc.send_cmd("channel", 30))
