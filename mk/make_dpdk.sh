#!/bin/sh

#
# usage
#   mk/make_dpdk.sh [opts] $(TOPDIR) $(DPDKDIR) $(RTE_ARCH) $(RTE_OS) $(CC)
#

TOPDIR=$1
DPDKDIR=$2
if test -z "${TOPDIR}" -o -z "${DPDKDIR}"; then
    exit 1
fi

RTE_ARCH=$3
if test -z "${RTE_ARCH}"; then
    # do not configure with dpdk, exit.
    exit 0
fi

RTE_OS=$4
CC=$5
if test -z "${RTE_OS}" -o -z "${CC}"; then
    exit 1
fi

git submodule update --init
if test $? -ne 0; then
    exit 1
fi
cd "${TOPDIR}/${DPDKDIR}"
if test $? -ne 0; then
    exit 0
fi

if [ "x${RTE_OS}" = "xbsdapp" ]; then
    MAKE=gmake
else
    MAKE=make
fi

RTE_TARGET=${RTE_ARCH}-native-${RTE_OS}-${CC}
NEW_TARGET=${RTE_ARCH}-native-lagopusapp-${CC}

cat > config/defconfig_$NEW_TARGET <<EOF
CONFIG_RTE_BUILD_SHARED_LIB=y
CONFIG_RTE_BUILD_COMBINE_LIBS=y
#CONFIG_RTE_PCI_EXTENDED_TAGS=on
#CONFIG_RTE_LIBRTE_PMD_QAT=y
#CONFIG_RTE_LIBRTE_PMD_AESNI_MB=y
CONFIG_RTE_LIBRTE_PMD_PCAP=n
CONFIG_RTE_APP_TEST=n
CONFIG_RTE_TEST_PMD=n
CONFIG_RTE_IXGBE_INC_VECTOR=n
EOF

grep -v include config/defconfig_${RTE_TARGET} \
    >> config/defconfig_${NEW_TARGET}
if test $? -ne 0; then
    exit 1
fi
egrep -v "(SHARED_LIB|COMBINE_LIB|PMD_PCAP|APP_TEST|TEST_PMD|IXGBE_INC_VECTOR)" config/common_${RTE_OS} \
    >> config/defconfig_${NEW_TARGET}
if test $? -ne 0; then
    exit 1
fi

${MAKE} T=${NEW_TARGET} config && ${MAKE} && \
    RTE_SDK="${TOPDIR}/${DPDKDIR}" \
    RTE_TARGET=build \
    RTE_OUTPUT=build \
    BUILDDIR=build \
    ${MAKE} -f ./lib/Makefile sharelib
