#!/usr/bin/env python

import sys
import socket
import ssl
import os
import select
import json
import logging
import asyncore
import threading
import six
from six.moves import _thread
from six.moves import queue
from contextlib import contextmanager

from const import *


class AsyncDataStoreCmd(asyncore.dispatcher):

    def __init__(self, host="127.0.0.1", port=12345, is_tls=False,
                 certfile=None, keyfile=None, ca_certs=None):
        asyncore.dispatcher.__init__(self)
        self.is_tls = is_tls
        if self.is_tls:
            self.certfile = certfile
            self.keyfile = keyfile
            self.ca_certs = ca_certs
        self.host = host
        self.port = port
        self.wbuf = b""
        self.queue = queue.Queue()
        self.th = None

    def create_sock(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        if self.is_tls:
            ssl_sock = ssl.wrap_socket(
                sock,
                certfile=self.certfile,
                keyfile=self.keyfile,
                ca_certs=self.ca_certs,
                cert_reqs=ssl.CERT_REQUIRED)
            sock = ssl_sock
        sock.setblocking(0)
        self.set_socket(sock)

    def connect(self):
        asyncore.dispatcher.connect(self, (self.host, self.port))
        proto = "TLS" if self.is_tls else "TCP"
        logging.info(
            "connected: " + self.host + ":" + str(self.port) + "(" + proto + ")")

    def handle_close(self):
        self.close()

    def writable(self):
        return ((len(self.wbuf) > 0) or (not self.queue.empty()))

    def handle_write(self):
        try:
            if len(self.wbuf) == 0:
                self.wbuf = self.queue.get_nowait()
                if self.wbuf is None:
                    _thread.exit()

            w = self.wbuf
            if six.PY3:
                w = self.wbuf.encode()

            sentlen = self.send(w)
            self.wbuf = self.wbuf[sentlen:]
        except queue.Empty:
            pass

    def readable(self):
        return True

    def handle_read(self):
        # ignore
        data = self.recv(BUFSIZE)
        if not data:
            raise RuntimeError("connection broken!")

        logging.debug("rcve: %s" % data)

    def send_cmd(self, cmd):
        if cmd is not None:
            cmd += "\n"
        self.queue.put(cmd)

    def loop(self):
        asyncore.loop(timeout=0.1)

    def run(self):
        self.th = threading.Thread(target=self.loop)
        self.th.start()

    def join(self):
        self.th.join()

    def is_alive(self):
        self.th.is_alive()


@contextmanager
def open_async_ds_cmd(**kwds):
    try:
        adsc = AsyncDataStoreCmd(**kwds)
        adsc.create_sock()
        adsc.connect()
        adsc.run()
        yield adsc
    finally:
        adsc.send_cmd(None)
        adsc.join()
        adsc.close()

if __name__ == "__main__":
    # tests
    # precondition: start lagopus.
    with open_async_ds_cmd() as adsc:
        adsc.send_cmd("channel cahnnel01 create")
        adsc.send_cmd("channel")
