#!/bin/sh

RTE_SDK=$1
PKG_CTRLFILE="debian/control.in"
PKG_COMMON_CTRLFILE="debian/control_common.inc"
PKG_RS_CTRLFILE="debian/control_raw_socket.inc"
PKG_DPDK_CTRLFILE="debian/control_dpdk.inc"

if test ! -f "${PKG_COMMON_CTRLFILE}" -o ! -f "${PKG_RS_CTRLFILE}" \
        -o ! -f "${PKG_DPDK_CTRLFILE}"; then
    exit 1
fi

cat ${PKG_COMMON_CTRLFILE} > ${PKG_CTRLFILE}
echo "" >> ${PKG_CTRLFILE}
cat ${PKG_RS_CTRLFILE} >> ${PKG_CTRLFILE}
if test "x${RTE_SDK}" != "x"; then
    echo "" >> ${PKG_CTRLFILE}
    cat ${PKG_DPDK_CTRLFILE} >> ${PKG_CTRLFILE}
fi

exit 0
