#/bin/sh


#


tmp1=/tmp/.wb1.$$
tmp2=/tmp/.wb2.$$
dirCache=/tmp/.dc.$$


> ${dirCache}


#


cleanup() {
    rm -f ${tmp1} ${tmp2} ${dirCache}
}


getCurDir() {
    tail -1 "${dirCache}"
    return 0
}


pushDir() {
    if test $# -ne 1; then
	return 1
    fi
    echo "${1}" >> "${dirCache}"
    echo "${1}"
    return 0
}


popDir() {
    _t=`getCurDir`
    if test ! -z "${_t}"; then
	if test $# -eq 1; then
	    if test "x${_t}" != "x${1}"; then
		return 1
	    fi
	fi
	head -n -1 "${dirCache}" > "${dirCache}.tmp"
	if test -r "${dirCache}.tmp"; then
	    rm -f "${dirCache}"
	    mv -f "${dirCache}.tmp" "${dirCache}"
	fi
    fi
    getCurDir
    return 0
}


parseMakeOut() {
    if test $# -ne 2; then
	return 1
    fi

    curDir=''
    while read c1 c2 c3 c4; do
	case ${c1} in
	    make\[*)
		d=`echo ${c4} | sed -e 's:[\`]::g' -e "s:[\']::g"`
		case ${c2} in
		    Entering)
			curDir=`pushDir "${d}"`;;
		    Leaving)
			curDir=`popDir "${d}"`;;
		esac;;
	    *:)
		if test "x${c2}" = "xwarning:"; then
		    file=`echo ${c1} | awk -F: '{ print $1 }' | \
			sed 's:\./::'`
		    dir=`dirname ${file}`
		    if test "x${dir}" = "x."; then
			file="${curDir}/${file}"
		    fi
		    z=`git log ${file} 2>/dev/null`
		    if test $? -eq 0 -a "x${z}" != "x"; then
			line=`echo ${c1} | awk -F: '{ print $2 }'`
			echo "${file} ${line}"
		    fi
		fi;;
	esac
    done < "${1}" | sort -k 1,1 -k 2,2n | uniq > ${2}

    return $?
}


blameIt() {
    if test $# -ne 2; then
	return 1
    fi

    cwd=`/bin/pwd`/
    curFile=''
    while read file line; do
	if test "x${curFile}" != "x${file}"; then
	    curFile="${file}"
	    title=`echo ${file} | sed "s:${cwd}::"`
	    echo
	    echo "${title}:"
	fi
	git blame -e -l -L ${line},${line} ${curFile}
    done < ${1} > ${2}

    return 0
}


#


trap cleanup 1 2 3 15

if test $# -ne 1; then
    exit 1
fi
if test ! -r ${1}; then
    exit 1
fi

parseMakeOut ${1} ${tmp1}
if test $? -ne 0 -o ! -r ${tmp1}; then
    cleanup
    exit 1
fi

blameIt ${tmp1} ${tmp2}
if test $? -ne 0 -o ! -r ${tmp2}; then
    cleanup
    exit 1
fi

cat ${tmp2}

cleanup

exit 0
