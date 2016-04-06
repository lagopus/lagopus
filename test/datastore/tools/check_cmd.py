#!/usr/bin/env python

# - lagopus.dsl
#   datastore -addr 127.0.0.1 -port 12345 -protocol tcp -tls true
#
#   tls -cert-file /<path to lagopus>/test/cert_for_test/server1.pem \
#       -private-key /<path to lagopus>/test/cert_for_test/server1_key_nopass.pem \
#       -certificate-store /<path to lagopus>/test/cert_for_test \
#       -trust-point-conf /<path to lagopus>/test/cert_for_test/checkcert.conf
#
# - s_client
#   openssl s_client -tls1 -CAfile cacert.pem -cert client1.pem \
#            -key client1_key_nopass.pem -connect 127.0.0.1:12345

import sys
import six
import readline
import socket
import ssl
import os
import select
from json import JSONDecoder
from argparse import ArgumentParser
from contextlib import closing
from six.moves import input

HOST = '127.0.0.1'
PORT = 12345
BUFSIZE = 4096

DIR = os.path.dirname(os.path.abspath(__file__))
CERT_DIR = DIR + "/../../cert_for_test"
CERTFILE = CERT_DIR + "/client1.pem"
KEYFILE = CERT_DIR + "/client1_key_nopass.pem"
CA_CERTS = CERT_DIR + "/cacert.pem"


def create_sock(opts):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if opts.is_tls:
        ssl_sock = ssl.wrap_socket(sock,
                                   certfile=opts.certfile,
                                   keyfile=opts.keyfile,
                                   ca_certs=opts.ca_certs,
                                   cert_reqs=ssl.CERT_REQUIRED)
        sock = ssl_sock
    return sock


def connect(sock, opts):
    sock.connect((opts.host, opts.port))
    proto = "TLS" if opts.is_tls else "TCP"
    six.print_("connected: " + opts.host + ":" + str(opts.port) + "(" + proto + ")")


def is_read_more(sock):
    return select.select([sock], [], [], 0) == ([sock], [], [])


def recvall(sock, opts):
    data = b""
    part = b""
    while True:
        part = sock.read(BUFSIZE) if opts.is_tls else sock.recv(BUFSIZE)

        p = part
        if six.PY3:
            p = part.decode()
        sys.stdout.write(p)
        sys.stdout.flush()

        data += part
        if not is_read_more(sock):
            d = data
            if six.PY3:
                d = data.decode()
            try:
                JSONDecoder().raw_decode(d)
                break
            except ValueError:
                pass


def tls_sendall(sock, cmd):
    sentlen = 0
    while sentlen < len(cmd):
        sent = sock.write(cmd[sentlen:])
        if sent == 0:
            raise RuntimeError("connection broken!")
        sentlen += sent


def send_cmd(sock, cmd, opts):
    cmd += "\n"
    c = cmd
    if six.PY3:
        c = cmd.encode()
    if opts.is_tls:
        tls_sendall(sock, c)
    else:
        sock.sendall(c)
    recvall(sock, opts)


def parse_opts():
    parser = ArgumentParser(description="DataStore cmd sender.")

    parser.add_argument("--host",
                        action="store",
                        type=str,
                        dest="host",
                        default=HOST,
                        help="host name")

    parser.add_argument("--tls",
                        action="store_true",
                        dest="is_tls",
                        default=False,
                        help="tls")

    parser.add_argument("--port",
                        action="store",
                        type=int,
                        dest="port",
                        default=PORT,
                        help="port no")

    parser.add_argument("--certfile",
                        action="store",
                        type=str,
                        dest="certfile",
                        default=CERTFILE,
                        help="certfile")

    parser.add_argument("--keyfile",
                        action="store",
                        type=str,
                        dest="keyfile",
                        default=KEYFILE,
                        help="keyfile")

    parser.add_argument("--ca_certs",
                        action="store",
                        type=str,
                        dest="ca_certs",
                        default=CA_CERTS,
                        help="CA certs")

    return parser.parse_args()

if __name__ == "__main__":
    readline.parse_and_bind('set editing-mode emacs')

    opts = parse_opts()

    sock = create_sock(opts)
    connect(sock, opts)

    with closing(sock):
        while True:
            line = input('> ')
            line = line.strip()
            if line == 'quit':
                break
            elif line == "":
                continue
            send_cmd(sock, line, opts)
