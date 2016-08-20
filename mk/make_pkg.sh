#!/bin/sh

# ./make_pkg.sh <TOP DIRECTORY> <PACKAGE DIRECTORY>

TOPDIR=$1
PKGDIR=$2

if [ "x${TOPDIR}" = "x" -o "x${PKGDIR}" = "x" ]; then
    echo "ERROR : Invalid args." 1>&2
    exit 1;
fi

PKGWORKDIR=lagopus
TMP_TAR="/tmp/lagopus_pkg_$$.tar.gz"

. ${TOPDIR}/mk/pkg_param.conf ${TOPDIR}

#

cleanup() {
    rm -f ${TMP_TAR}
    if [ "x${1}" = "xTRAP" ]; then
        rm -rf ${PKGDIR}/
    fi
}

trap 'cleanup "TRAP"' 1 2 3 15

#

if [ "x${PKG_NAME}" = "x" -o "x${PKG_VERSION}" = "x" -o \
     "x${PKG_REVISION}" = "x" -o "x${PKG_MAINTAINER}" = "x" -o  \
     "x${PKG_SECTION}" = "x" -o "x${PKG_PRIORITY}" = "x" ]; then
    echo "ERROR : Required fields:" 1>&2
    echo "  PKG_{NAMEPKG, VERSION, REVISION," 1>&2
    echo "       MAINTAINER, SECTION, PRIORITY}." 1>&2
    exit 1
fi

mkdir -p ${PKGDIR}/${PKGWORKDIR}
cd ${TOPDIR} && tar czf ${TMP_TAR} ./ --exclude .git*
mv ${TMP_TAR} ${PKGDIR}/lagopus_${PKG_VERSION}.orig.tar.gz
cd ${PKGDIR} && tar zxf lagopus_${PKG_VERSION}.orig.tar.gz -C ${PKGWORKDIR}
cd ${PKGDIR}/${PKGWORKDIR} &&
    DEB_CPPFLAGS_SET= DEB_CFLAGS_SET= DEB_LDFLAGS_SET= \
    DEB_CXXFLAGS_SET= DEB_FFLAGS_SET= \
    dpkg-buildpackage -uc -us -rfakeroot

cleanup

exit 0
