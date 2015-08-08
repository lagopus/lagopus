#!/bin/bash

NAME="ptree"
VERSION="1.0.1"
TMP_DIR="/tmp/rpmbuild-${NAME}.$$"

SCRIPT="$(readlink -f $0)"
SCRIPT_DIR="$(dirname ${SCRIPT})"
BASE_DIR="$(readlink -f ${SCRIPT_DIR}/..)"

function topdir() {
    local path="$1"; shift
    grep '%_topdir' "${path}" 2> /dev/null | awk '{print $2}'
}

function main() {
    local rpmmacros="$(readlink -f ~/.rpmmacros)"
    local topdir="$(topdir ${rpmmacros})"
    local specdir="${topdir}/SPECS"
    local srcdir="${topdir}/SOURCES"

    if [ -z "${topdir}" ]; then
        echo "%_topdir undefined. please check ${rpmmacros}" 1>&2
        exit 1
    elif [ ! -d "${specdir}" ]; then
        echo "${specdir} is not a directory." 1>&2
        exit 2
    elif [ ! -d "${srcdir}" ]; then
        echo "${srcdir} is not a directory." 1>&2
        exit 3
    fi

    mkdir -p "${TMP_DIR}"
    rsync -a "${BASE_DIR}/" "${TMP_DIR}/${NAME}-${VERSION}"
    tar -C "${TMP_DIR}" -jcf "${TMP_DIR}/${NAME}-${VERSION}.tar.bz2" "${NAME}-${VERSION}"
    cp "${TMP_DIR}/${NAME}-${VERSION}.tar.bz2" "${srcdir}/${NAME}-${VERSION}.tar.bz2"
    rm -rf "${TMP_DIR}"

    cp "${SCRIPT_DIR}/${NAME}.spec" "${specdir}/${NAME}.spec"
    rpmbuild -bb "${specdir}/${NAME}.spec"
}

main $@
