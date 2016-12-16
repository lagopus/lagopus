#!/bin/sh

#
# usage
#   mk/make_dpdk.sh [opts] $(TOPDIR) $(DPDKDIR) $(RTE_ARCH) $(RTE_OS) $(CC)
#

#
# usage
#  edit_dpdk_config VAR=value file
#
edit_dpdk_config() {
    NEWLINE="$1"
    FILE="$2"
    VAR="${NEWLINE%%=*}"
    grep "^${VAR}=" $FILE > /dev/null
    if [ "$?" = 0 ]; then
	sed "s/^${VAR}=.*\$/$NEWLINE/" $FILE > $FILE.tmp
    else
	cp $FILE $FILE.tmp
	echo "$NEWLINE" >> $FILE.tmp
    fi
    if test $? -ne 0; then
	exit 1
    fi
    mv $FILE.tmp $FILE
    if test $? -ne 0; then
	exit 1
    fi
}

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

git submodule sync
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

NEWCONFIG=config/defconfig_$NEW_TARGET

cp config/defconfig_${RTE_TARGET} $NEWCONFIG
if test $? -ne 0; then
    exit 1
fi

edit_dpdk_config CONFIG_RTE_BUILD_COMBINE_LIBS=y $NEWCONFIG
edit_dpdk_config CONFIG_RTE_BUILD_SHARED_LIB=y $NEWCONFIG
#edit_dpdk_config CONFIG_RTE_PCI_EXTENDED_TAGS=on $NEWCONFIG
#edit_dpdk_config CONFIG_RTE_LIBRTE_PMD_QAT=y $NEWCONFIG
#edit_dpdk_config CONFIG_RTE_LIBRTE_PMD_AESNI_MB=y $NEWCONFIG
edit_dpdk_config CONFIG_RTE_IXGBE_INC_VECTOR=n $NEWCONFIG
edit_dpdk_config CONFIG_RTE_LIBRTE_PMD_PCAP=n $NEWCONFIG
edit_dpdk_config CONFIG_RTE_LIBRTE_PMD_VHOST=y $NEWCONFIG
edit_dpdk_config CONFIG_RTE_LIBRTE_PMD_NULL_CRYPTO=n $NEWCONFIG
edit_dpdk_config CONFIG_RTE_LIBRTE_PMD_BOND=n $NEWCONFIG
edit_dpdk_config CONFIG_RTE_APP_TEST=n $NEWCONFIG
edit_dpdk_config CONFIG_RTE_TEST_PMD=n $NEWCONFIG

${MAKE} T=${NEW_TARGET} config && ${MAKE} ROOTDIRS-y="buildtools lib drivers"
