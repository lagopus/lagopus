# How to Run DPDK with virtio_user in Docker container

## Introduction

The virtio_user driver enables DPDK application to connect with the vhost_user driver.
It is designed for connecting vhost application with containerized DPDK app.

This document shows how to run DPDK apps with the virtio_user driver.

- Reference
  - http://dpdk.org/doc/guides/howto/virtio_user_for_container_networking.html
  - http://dpdk.org/ml/archives/dev/2016-June/040985.html
  - https://dpdksummit.com/Archive/pdf/2016USA/Day02-Session02-Steve%20Liang-DPDKUSASummit2016.pdf


## Points of using virtio_user driver

The difference of the general instruction is below.

- Use **1GB size hugepages** and allocate only **less than or equal to 8 pages** to hugepages.
  - or else, it can't work.
  - This problem comes from a limitation of the virtio_user driver.
  - >Doesn't work when there are more than ``VHOST_MEMORY_MAX_NREGIONS(8)`` hugepages. (http://dpdk.org/doc/guides/rel_notes/release_16_07.html)
- Allocate hugetlbfs to each container.
- Mount hugetlbfs to each container.
- Mount the vhost socket to container.
- Set the `-m` EAL option to avoid allocating entire memory.
  - For sharing hugepages with each DPDK apps.

---

## Example: Running `testpmd` <-> `testpmd`

This example runs `testpmd` on both the host and the container.


### Preparation

- The way to allocate 1GB size hugepages is [here](how-to-allocate-1gb-hugepages).
- The way to build `testpmd` is [here]( http://dpdk.org/doc/guides/testpmd_app_ug/build_app.html)


### Topology

```
                                        Docker Container
                                      +------------------+
  +---------------+                   | +--------------+ |
  |               |                   | |              | |
  |    eth_vhost0 +>-------------------<+ virtio_user0 | |
  |               |                   | |              | |
  |    testpmd    |                   | |   testpmd    | |
  |               |                   | |              | |
  |    eth_vhost1 +>-------------------<+ virtio_user1 | |
  |               |                   | |              | |
  +---------------+                   | +--------------+ |
                                      +------------------+
  +------------------------------------------------------+
  |                       Kernel                         |
  +------------------------------------------------------+
```


### Run testpmd on host


```console
$ sudo mkdir -p /tmp/dpdk
$ sudo rm -rf /tmp/dpdk/* # when you run again
$ sudo /path/to/testpmd --no-pci --vdev=eth_vhost0,iface=/tmp/dpdk/sock0 --vdev=eth_vhost1,iface=/tmp/dpdk/sock1 -c 0xc -n 2 -m 1024
```

### Run testpmd on container

When the connection is collectly established, these counters of `RX-packets` and `TX-packates` should not be zero.

- `Dockerfile`

```Dockerfile
FROM ubuntu:16.04

ARG DPDK_VERSION=16.11

# Install Required packages
RUN apt-get update -y \
    && apt-get install -y -q curl build-essential linux-headers-$(uname -r) \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Download DPDK source code
WORKDIR /tmp
RUN curl -O http://fast.dpdk.org/rel/dpdk-${DPDK_VERSION}.tar.xz \
       && tar xJf dpdk-${DPDK_VERSION}.tar.xz -C /usr/local/src \
       && rm dpdk-${DPDK_VERSION}.tar.xz

ENV RTE_TARGET x86_64-native-linuxapp-gcc
ENV RTE_SDK /usr/local/src/dpdk-${DPDK_VERSION}

# Build
WORKDIR $RTE_SDK
RUN make install -j${nproc} T=${RTE_TARGET} \
    && make -C examples/ -j${nproc}

# Set PATH for app
ENV PATH "$PATH:$RTE_SDK/$RTE_TARGET/app"
```

```console
$ docker build -t dpdk-docker .
$ docker run -it --rm -v /mnt/huge_c0:/mnt/huge_c0 -v /tmp/dpdk:/tmp/dpdk dpdk-docker testpmd --no-pci --vdev=virtio_user0,path=/tmp/dpdk/sock0 --vdev=virtio_user1,path=/tmp/dpdk/sock1 -c 0x3 -n 2 -m 1024  -- --disable-hw-vlan-filter -i
testpmd> start tx_first
...
testpmd> stop
Telling cores to stop...
Waiting for lcores to finish...

  ---------------------- Forward statistics for port 0  ----------------------
  RX-packets: 4652768        RX-dropped: 0             RX-total: 4652768
  TX-packets: 2327104        TX-dropped: 0             TX-total: 2327104
  ----------------------------------------------------------------------------

  ---------------------- Forward statistics for port 1  ----------------------
  RX-packets: 2327072        RX-dropped: 0             RX-total: 2327072
  TX-packets: 4652800        TX-dropped: 0             TX-total: 4652800
  ----------------------------------------------------------------------------

  +++++++++++++++ Accumulated forward statistics for all ports+++++++++++++++
  RX-packets: 6979840        RX-dropped: 0             RX-total: 6979840
  TX-packets: 6979904        TX-dropped: 0             TX-total: 6979904
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Done.
```

---

## Example: Running `Lagopus` <-> `testpmd`

This example runs Lagopus as vhost and testpmd as virtio_user.

### Preparation

The difference between Lagopus and tespmd is only configuration of Lagopus.
You can run only setting `lagopus.dsl` shown below.

```
channel channel01 create -dst-addr 127.0.0.1 -protocol tcp
controller controller01 create -channel channel01 -role equal -connection-type main

interface interface01 create -type ethernet-dpdk-phy -device eth_vhost0,iface=/tmp/dpdk/sock0
interface interface02 create -type ethernet-dpdk-phy -device eth_vhost1,iface=/tmp/dpdk/sock1

port port01 create -interface interface01
port port02 create -interface interface02

bridge bridge01 create -controller controller01 -port port01 1 -port port02 2 -dpid 0x1
bridge bridge01 enable

flow bridge01 add in_port=1 apply_actions=output:2
flow bridge01 add in_port=2 apply_actions=output:1
```


### Topology


```
                                        Docker Container
                                      +------------------+
  +---------------+                   | +--------------+ |
  |               |                   | |              | |
  |    eth_vhost0 +>-------------------<+ virtio_user0 | |
  |               |                   | |              | |
  |    Lagpous    |                   | |   testpmd    | |
  |               |                   | |              | |
  |    eth_vhost1 +>-------------------<+ virtio_user1 | |
  |               |                   | |              | |
  +---------------+                   | +--------------+ |
                                      +------------------+
  +------------------------------------------------------+
  |                       Kernel                         |
  +------------------------------------------------------+
```

### Run Lagpous on host

```console
$ sudo mkdir -p /tmp/dpdk
$ sudo rm -rf /tmp/dpdk/* # when you run again
$ sudo lagopus -d -- -c 0xc -n 2 -m 1024
```

### Run testpmd on container

This is same way as `testpmd` <-> `testpmd` example.
