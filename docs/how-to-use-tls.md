<!-- -*- mode: markdown ; coding: us-ascii-dos -*- -->

How To Use TLS 
======================================

Introduction
------------
This introduce how to use TLS in Lagopus switch to ensure secure
connection to an OpenFlow controller.

Certification setup
---------------------------
Before Lagopus setup, you have to prepare CA and X.509-based certificates.

Please refer the following web sites:

* http://pki-tutorial.readthedocs.org/en/latest/simple/
* https://help.ubuntu.com/community/OpenSSL
* http://pages.cs.wisc.edu/~zmiller/ca-howto/

Lagopus setup
---------------------------
To use TLS for a channel between Lagopus and an OpenFlow controller,
we should setup both "lagopus.dsl" and "check.conf".

### lagopus.dsl
To enable TLS on Lagopus, you have to add a tls directive and
tls option in a channel directive in lagopus.dsl.
You do not need to modify other configuration.

```
tls -cert-file <certificate store path> -private-key <private key PEM file> -certificate-store <certification PEM file> -trust-point-conf <check.conf file>
channel channel01 create -dst-addr 127.0.0.1 -protocol tls
```

#### TLS-related option

* cert-file:
  To speicfy a certificate store path. (default: /etc/lagopus)
* private-key: To speicfy a PEM file that contains a private key. (default: /etc/lagopus/catls.pem)
* certificate-store: To specify a certification PEM file. (default: /etc/lagopus/key.pem)
* trust-point-conf: TO specify a trust point check configuration file. (default: /etc/lagopus/check.conf)

## Trust point check configuration file

You can specify the configuration of trust point check to accept a certification of an OpenFlow controller.
The description of trust point check consists of two commands, "allow" and "deny". in line basis. 
These commands include "issuer" and "subject", or "self-signed"
("selfsigned" is also OK) to check X.509-based certificate.
Regular expression is allowed in the description of issuer and subject.

The following description is an example file of trust point check configuration.

```
deny self-signed

allow issuer '/C=JP/ST=Somewhere/L=Somewhere/O=Somewhere/OU=Somewhere/CN=Middle of Nowhere\'s tier I CA/emailAddress=ca@example\.net' \
      subject "/C=JP/ST=Somewhere/L=Somewhere/O=Somewhere/OU=Somewhere/CN=John \"774\" Doe/emailAddress=john\.774\.doe@example\.net"

allow issuer "/C=JP*" subject "/C=JP*"
```

Limitation
------------
Current implementation of Lagopus only support single CA.
Future we will support multiple CAs for each OpenFlow channel.

Reference
------------
* http://ryu.readthedocs.org/en/latest/tls.html

