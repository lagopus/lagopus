##
## Autoconf template.
##
AC_INIT

TOPDIR=${ac_pwd}
AC_SUBST(TOPDIR)

AC_CONFIG_AUX_DIR(./mk)
AC_CONFIG_MACRO_DIR(./mk)

AC_CONFIG_HEADER([src/include/lagopus_config.h])

if test "${prefix}" = "NONE"; then
    prefix=/usr/local
fi
if test "${exec_prefix}" = "NONE"; then
    exec_prefix='${prefix}'
fi

PREFIX=${prefix}
AC_SUBST(PREFIX)

case `uname` in
Linux)
    SHELL=`which bash` > /dev/null 2>&1
    if test $? -ne 0 -o -z "${SHELL}"; then
        AC_MSG_ERROR([To run libtool you need the GNU bash, not the old-school /bin/sh.])
        exit 1
    fi
;;
*)
    AC_MSG_RESULT([SHELL is "$SHELL". bash is recommended.])
;;
esac

export SHELL
AC_SUBST(SHELL)

LT_INIT
AC_SUBST([LIBTOOL_DEPS])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_YACC
AC_PROG_LEX
if test "x$LEX" != xflex -a "x$LEX" != xlex; then
   AC_MSG_ERROR([flex/lex must be installed.])
fi
AC_PROG_AWK
AC_TYPE_SIGNAL

RM="rm -f"
AC_SUBST(RM)

MKDIR="mkdir -p"
AC_SUBST(MKDIR)

LN_S="ln -nfs"
AC_SUBST(LN_S)

READLINK="readlink"
AC_SUBST(READLINK)

BASENAME="basename"
AC_SUBST(BASENAME)

# Assume 32 bit for cross compilation.
AC_CHECK_SIZEOF(short int, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long int, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(int64_t, 8)
AC_CHECK_SIZEOF(size_t, 4)
AC_CHECK_SIZEOF(void *, 4)
AC_CHECK_SIZEOF(pthread_t, 4)

# sysdeps.

eval `echo ${target} | awk -F- '{ printf "tagCPU=%s; tagOS=%s;\n", $1, $3 }'`

changequote(<<, >>)dnl
CPU=`echo ${tagCPU} | tr '[A-Z]' '[a-z]'`
OS=`echo ${tagOS} | tr '[A-Z]' '[a-z]'`
changequote([, ])dnl
unset tagOS tagCPU

case $CPU in
    i*86)
        CPU=i386;;
esac

OS_CPPFLAGS='-D_REENTRANT'
OS_LDFLAGS=''
OS_LIBS=''
OS_CFLAGS=''
if test -z "${MAKE}"; then
	MAKE=make
fi

case $OS in
    sunos*)
        OS=sunos;;
    solaris*)
        OS=solaris;;
    freebsd*)
	MAKE=gmake
	OS_CPPFLAGS="${OS_CPPFLAGS} -I/usr/local/include"
	OS_LDFLAGS="${OS_LDFLAGS} -L/usr/local/lib"
	CPPFLAGS="${CPPFLAGS} ${OS_CPPFLAGS}"
	LDFLAGS="${LDFLAGS} ${OS_LDFLAGS}"
	OS_LIBS="-lpthread -lexecinfo"
	OS=freebsd;;
    linux*)
	OS_CPPFLAGS="${OS_CPPFLAGS} -D_GNU_SOURCE -D_POSIX_SOURCE"
	OS_LIBS="-lrt -lpthread -ldl"
	OS=linux;;
    netbsd*)
	MAKE=gmake
	OS_CPPFLAGS="${OS_CPPFLAGS} -I/usr/pkg/include"
	OS_LDFLAGS="${OS_LDFLAGS} -L/usr/pkg/lib"
	CPPFLAGS="${CPPFLAGS} ${OS_CPPFLAGS}"
	LDFLAGS="${LDFLAGS} ${OS_LDFLAGS}"
	OS_LIBS="-lpthread -lexecinfo"
	OS=netbsd;;
esac

AC_SUBST(MAKE)
AC_SUBST(OS_CPPFLAGS)
AC_SUBST(OS_CFLAGS)
AC_SUBST(OS_LDFLAGS)
AC_SUBST(OS_LIBS)

changequote(<<, >>)dnl
CPUDEF=LAGOPUS_CPU_`echo $CPU | tr '[a-z]' '[A-Z]'`
OSDEF=LAGOPUS_OS_`echo $OS | tr '[a-z]' '[A-Z]'`
changequote([, ])dnl
AC_SUBST(CPUDEF)
AC_SUBST(OSDEF)

MKRULESDIR=${TOPDIR}/mk
AC_SUBST(MKRULESDIR)

AC_SUBST(CPPFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(TARGET_ARCH)

RUBY=ruby
NKF=nkf
UNITY_SRC_DIR=

AC_ARG_ENABLE([developer],
	[AC_HELP_STRING([--enable-developer],
	 [enable build for developers, like unit tests [default is no]])]
	 ,[developer="$withval"], [developer=no])

if test "X${developer}" != "Xno"; then
  AC_CHECK_PROG([HAVE_NKF], [nkf], [yes], [])

  AC_CHECK_PROG([HAVE_RUBY], [ruby], [yes], [no])

  if test "X${HAVE_RUBY}" != "Xyes"; then
    AC_MSG_ERROR([developer's build requires ruby.])
  fi

  AC_CHECK_PROG([HAVE_JQ], [jq], [yes], [no])

  if test "X${HAVE_JQ}" != "Xyes"; then
    AC_MSG_ERROR([developer's build requires jq (Command-line JSON processor).])
  fi

  ./mk/unitycheck.sh ${CC}
  if test $? -eq 0 ; then
    HAVE_UNITY=yes
    UNITY_SRC_DIR=unity/src
    AC_MSG_RESULT([unity is available.])
  else
    HAVE_UNITY=
    AC_MSG_ERROR([Unity is not available. developer's build requires Unity. Please check https://github.com/lagopus/lagopus/blob/master/docs/how-to-write-tests-in-lagopus.md.])
  fi

  AC_CHECK_PROG([HAVE_GCOVR], [gcovr], [yes],)
  if test "X${HAVE_GCOVR}" != "Xyes"; then
    AC_MSG_ERROR([taking coverage requires gcovr..])
  fi
fi

AC_SUBST(HAVE_RUBY)
AC_SUBST(HAVE_UNITY)
AC_SUBST(UNITY_SRC_DIR)
AC_SUBST(HAVE_NKF)
AC_SUBST(HAVE_GCOVR)

AC_MSG_CHECKING([/proc/self/exe])
if test -h /proc/self/exe; then
  AC_MSG_RESULT([exists.])
  AC_DEFINE(HAVE_PROCFS_SELF_EXE)
else
  AC_MSG_RESULT([does noe exists.])
fi

AC_ARG_ENABLE([hybrid],
	[AC_HELP_STRING([--enable-hybrid],
	 [enable build as hybrid switch [default is no]])]
	 ,[hybrid="$enableval"], [hybrid=no])

if test "X${hybrid}" != "Xno"; then
   HYBRID_CPPFLAGS=-DHYBRID
fi
AC_SUBST(HYBRID_CPPFLAGS)

# headers/macros

oLIBS=${LIBS}
LIBS=''
AC_CHECK_HEADER(pcap/pcap.h,, [AC_MSG_ERROR([pcap/pcap.h is required!])])
AC_CHECK_LIB(pcap, pcap_create,, [AC_MSG_ERROR([libpcap is required!])])
PCAP_LIBS=${LIBS}
AC_SUBST(PCAP_LIBS)
LIBS=${oLIBS}

AC_C_BIGENDIAN

AC_HEADER_STDC

AC_CHECK_HEADER(inttypes.h, [AC_DEFINE(HAVE_INTTYPES_H)], [])
AC_CHECK_HEADER(stdint.h, [AC_DEFINE(HAVE_STDINT_H)])
AC_CHECK_HEADER(limits.h, [AC_DEFINE(HAVE_LIMITS_H)])
AC_CHECK_HEADER(sys/param.h, [AC_DEFINE(HAVE_SYS_PARAM_H)])
AC_CHECK_HEADER(sys/types.h, [AC_DEFINE(HAVE_SYS_TYPES_H)])
AC_CHECK_HEADER(ctype.h, [AC_DEFINE(HAVE_CTYPE_H)])
AC_CHECK_HEADER(stdio.h, [AC_DEFINE(HAVE_STDIO_H)])
AC_CHECK_HEADER(stdlib.h, [AC_DEFINE(HAVE_STDLIB_H)])
AC_CHECK_HEADER(unistd.h, [AC_DEFINE(HAVE_UNISTD_H)])
AC_CHECK_HEADER(memory.h, [AC_DEFINE(HAVE_MEMORY_H)])
AC_CHECK_HEADER(string.h, [AC_DEFINE(HAVE_STRING_H)])
AC_CHECK_HEADER(malloc.h, [AC_DEFINE(HAVE_MALLOC_H)])
AC_CHECK_HEADER(stdarg.h, [AC_DEFINE(HAVE_STDARG_H)])
AC_CHECK_HEADER(dirent.h, [AC_DEFINE(HAVE_DIRENT_H)])
AC_CHECK_HEADER(time.h, [AC_DEFINE(HAVE_TIME_H)])
AC_CHECK_HEADER(sys/time.h, [AC_DEFINE(HAVE_SYS_TIME_H)])
AC_CHECK_HEADER(termios.h, [AC_DEFINE(HAVE_TERMIOS_H)])
AC_CHECK_HEADER(sys/stat.h, [AC_DEFINE(HAVE_SYS_STAT_H)])
AC_CHECK_HEADER(fcntl.h, [AC_DEFINE(HAVE_FCNTL_H)])
AC_CHECK_HEADER(errno.h, [AC_DEFINE(HAVE_ERRNO_H)])
AC_CHECK_HEADER(signal.h, [AC_DEFINE(HAVE_SIGNAL_H)])
AC_CHECK_HEADER(netdb.h, [AC_DEFINE(HAVE_NETDB_H)])
AC_CHECK_HEADER(netinet/tcp.h, [AC_DEFINE(HAVE_NETINET_TCP_H)])
AC_CHECK_HEADER(sys/socket.h, [AC_DEFINE(HAVE_SYS_SOCKET_H)])
AC_CHECK_HEADER(arpa/inet.h, [AC_DEFINE(HAVE_ARPA_INET_H)])
AC_CHECK_HEADER(sys/ioctl.h, [AC_DEFINE(HAVE_SYS_IOCTL_H)])
AC_CHECK_HEADER(sys/wait.h, [AC_DEFINE(HAVE_SYS_WAIT_H)])
AC_CHECK_HEADER(pthread.h, [AC_DEFINE(HAVE_PTHREAD_H)])
AC_CHECK_HEADER(stdbool.h, [AC_DEFINE(HAVE_STDBOOL_H)])
AC_CHECK_HEADER(sys/resource.h, [AC_DEFINE(HAVE_SYS_RESOURCE_H)])
AC_CHECK_HEADER(regex.h, [AC_DEFINE(HAVE_REGEX_H)])
AC_CHECK_HEADER(syslog.h, [AC_DEFINE(HAVE_SYSLOG_H)])
AC_CHECK_HEADER(getopt.h, [AC_DEFINE(HAVE_GETOPT_H)])
AC_CHECK_HEADER(bsd/sys/queue.h, [AC_DEFINE(HAVE_BSD_SYS_QUEUE_H)])
AC_CHECK_HEADER(sys/queue.h, [AC_DEFINE(HAVE_SYS_QUEUE_H)])
AC_CHECK_HEADER(mcheck.h, [AC_DEFINE(HAVE_MCHECK_H)])
AC_CHECK_HEADER(termcap.h, [AC_DEFINE(HAVE_TERMCAP_H)])
AC_CHECK_HEADER(curses.h, [AC_DEFINE(HAVE_CURSES_H)])
AC_CHECK_HEADER(ncurses.h, [AC_DEFINE(HAVE_NCURSES_H)])
AC_CHECK_HEADER(assert.h, [AC_DEFINE(HAVE_ASSERT_H)])
AC_CHECK_HEADER(poll.h, [AC_DEFINE(HAVE_POLL_H)])
AC_CHECK_HEADER(sys/socket.h, [AC_DEFINE(HAVE_SYS_SOCKET_H)])
AC_CHECK_HEADER(net/ethernet.h, [AC_DEFINE(HAVE_NET_ETHERNET_H)])

oLIBS=${LIBS}
LIBS="${LIBS} -lpthread"
AC_CHECK_FUNCS([strlcpy strlcat, pthread_mutex_timedlock])
LIBS=${oLIBS}
AC_CHECK_FUNC(strtold, [AC_DEFINE(HAVE_STRTOLD)])


# snmp configuration

SNMP_DIR=
SNMP_CPPFLAGS=
DEP_SNMP_HANDLER_LIB=
NETSNMP_AGENT_LIBS=

AC_ARG_ENABLE([snmp],
	[AC_HELP_STRING([--enable-snmp],
	 [build with snmp feature [default is no]])]
	 ,[snmp="$enableval"], [snmp=no])

if test "X${snmp}" != "Xno"; then

   SNMP_DIR=src/snmp
   SNMP_CPPFLAGS=-DENABLE_SNMP_MODULE

   oLIBS=${LIBS}
   LIBS=''
   AC_CHECK_HEADERS([net-snmp/net-snmp-config.h\
                     net-snmp/net-snmp-includes.h\
                     net-snmp/agent/net-snmp-agent-includes.h\
                     net-snmp/library/snmp_logging.h],
                    [],
                    [AC_MSG_ERROR([netsnmp library headers must be installed])],
                    [[#ifndef NET_SNMP_CONFIG_H
#include <net-snmp/net-snmp-config.h>
# ifndef NET_SNMP_INCLUDES_H
#include <net-snmp/net-snmp-includes.h>
#  ifndef NET_SNMP_AGENT_INCLUDES_H
#include <net-snmp/agent/net-snmp-agent-includes.h>
#  endif
# endif
#endif
]])

    AC_CHECK_LIB(netsnmp, init_snmp, [], [AC_MSG_ERROR([libnetsnmp is required!])], [])
    AC_CHECK_LIB(netsnmpmibs, init_snmp_mib, [],
      [AC_CHECK_LIB(netsnmpmibs, init_snmp_mib_5_5, [],
        [AC_MSG_ERROR([libnetsnmpmibs is required!])], [-lnetsnmp])])
    AC_CHECK_LIB(netsnmphelpers, netsnmp_init_helpers, [],
      [AC_CHECK_LIB(netsnmpagent, netsnmp_init_helpers, [],
        [AC_MSG_ERROR([libnetsnmphelpers or libnetsnmpagent is required!])], [-lnetsnmpmibs -lnetsnmp])])
    AC_CHECK_LIB(netsnmpagent, init_agent, [], [AC_MSG_ERROR([libnetsnmpagent is required!])], [-lnetsnmphelpers -lnetsnmpmibs -lnetsnmp])
    NETSNMP_AGENT_LIBS=${LIBS}
    LIBS=${oLIBS}

    DEP_SNMP_HANDLER_LIB='$(DEP_SNMP_HANDLER_LIB)'
fi

AC_SUBST(SNMP_DIR)
AC_SUBST(SNMP_CPPFLAGS)
AC_SUBST(NETSNMP_AGENT_LIBS)
AC_SUBST(DEP_SNMP_HANDLER_LIB)

# end net-snmp configuration

# check ssl library
AC_CHECK_HEADER(openssl/ssl.h,, [AC_MSG_ERROR([openssl/ssl.h is required!])])
oLIBS=${LIBS}
SSL_LIBS=''
LIBS=''
AC_CHECK_LIB(ssl, SSL_new,, [AC_MSG_ERROR([libssl is required!])])
AC_CHECK_LIB(crypto, X509_new,, [AC_MSG_ERROR([libcrypto is required!])])
SSL_LIBS="${LIBS}"
AC_SUBST(SSL_LIBS)
LIBS=${oLIBS}

# check for gmp.
AC_CHECK_HEADER(gmp.h, [AC_DEFINE(HAVE_GMP_H)], [AC_MSG_ERROR([The GNU MP must be installed.])])
resultOK=0
GMP_LIBS=''
oLIBS=${LIBS}
LIBS='-lgmp'
AC_MSG_CHECKING([mpz_init_set_str])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <gmp.h>]],
		[[char *str = "12345678";]
		 [MP_INT i;]
		 [int r;]
		 [r = mpz_init_set_str(&i, str, 10);]
		 [return r;]])],
	[resultOK=1],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
LIBS=${oLIBS}
if test ${resultOK} -eq 1; then
    AC_MSG_RESULT([available.])
    GMP_LIBS='-lgmp'
else
    AC_MSG_ERROR([The GNU MP library must be installed.])
fi
AC_SUBST(GMP_LIBS)

AC_ARG_ENABLE([jumbo-frame],
	[AC_HELP_STRING([--enable-jumbo-frame],
	 [enable jumbo frame [default is no]])]
	 ,[jumbo_frame="$enableval"], [jumbo_frame=no])

if test "X${jumbo_frame}" != "Xno"; then
   FRAME_CPPFLAGS=-DMAX_PACKET_SZ=16384
else
   FRAME_CPPFLAGS=-DMAX_PACKET_SZ=2048
fi
AC_SUBST(FRAME_CPPFLAGS)

AC_ARG_WITH(dpdk-dir,
            [AC_HELP_STRING([--with-dpdk-dir=PATH],
	                    [specify top directory of extracted DPDK])],
            [],
            [with_dpdk_dir=""])
if test "x$with_dpdk_dir" != "x"; then
  RTE_SDK=$with_dpdk_dir
  RTE_TARGET=build
  AC_CHECK_HEADER(${RTE_SDK}/${RTE_TARGET}/include/rte_common.h, [],
    [RTE_ARCH=`uname -p`
     if test "x$RTE_ARCH" = "xunknown"; then
        RTE_ARCH=`uname -m`
     fi
     case `uname` in
     Linux)
     RTE_OS=linuxapp
     ;;
     FreeBSD)
     RTE_OS=bsdapp
     ;;
     *)
     AC_MSG_ERROR([DPDK must be supported your OS.])
     ;;
     esac
     RTE_TARGET=${RTE_ARCH}-native-${RTE_OS}-${CC}
     AC_CHECK_HEADER(${RTE_SDK}/${RTE_TARGET}/include/rte_common.h, [],
       [RTE_TARGET=${RTE_ARCH}-default-${RTE_OS}-${CC}
        AC_CHECK_HEADER(${RTE_SDK}/${RTE_TARGET}/include/rte_common.h, [],
          [AC_MSG_ERROR([DPDK must be installed.])])])])
  RTE_LIBS=-L$RTE_SDK/$RTE_TARGET/lib
  RTE_LIBS="$RTE_LIBS -Xlinker --whole-archive"
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_distributor.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_distributor",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_reorder.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_reorder",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_kni.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_kni",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_ivshmem.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_ivshmem",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pipeline.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pipeline",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_table.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_table",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_port.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_port",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_timer.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_timer",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_jobstats.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_jobstats",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_hash.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_hash",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_lpm.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_lpm",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_power.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_power",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_acl.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_acl",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_meter.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_meter",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_sched.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_sched -lm -lrt",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_vhost.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_vhost",)
  RTE_LIBS="$RTE_LIBS -Xlinker --start-group"
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_kvargs.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_kvargs",)
  RTE_LIBS="$RTE_LIBS -Xlinker -lrte_mbuf"
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_ip_frag.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_ip_frag",)
  RTE_LIBS="$RTE_LIBS -lrte_ring -lrte_eal"
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_cmdline.a,
                RTE_LIBS="$RTE_LIBS -lrte_cmdline",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_cfgfile.a,
                RTE_LIBS="$RTE_LIBS -lrte_cfgfile",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_bond.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_bond",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_xenvirt.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_xenvirt -lxenstore",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_vmxnet3.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_vmxnet3",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_virtio.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_virtio",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_vmxnet3_uio.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_vmxnet3_uio",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_virtio_uio.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_virtio_uio",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_enic.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_enic",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_i40e.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_i40e",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_fm10k.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_fm10k",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_ixgbe.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_ixgbe",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_e1000.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_e1000",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_mlx4.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_mlx4",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_ring.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_ring",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_pcap.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_pcap",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_af_packet.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_af_packet",)
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_null.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_pmd_null",)
  RTE_LIBS="$RTE_LIBS -Xlinker --end-group -Xlinker --no-whole-archive"
  RTE_LIBS="$RTE_LIBS -lethdev"
  AC_CHECK_FILE(${RTE_SDK}/${RTE_TARGET}/lib/librte_malloc.a,
                RTE_LIBS="$RTE_LIBS -Xlinker -lrte_malloc",)
  RTE_LIBS="$RTE_LIBS  -lrte_mempool"
  CPPFLAGS_BKUP="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS -I$RTE_SDK/$RTE_TARGET/include"
  AC_CHECK_MEMBER([struct rte_mbuf.pkt],
                  [RTE_MBUF_HAS_PKT=1],
		  [],
		  [[#include "rte_config.h"]
		   [#include "rte_mbuf.h"]])
  CPPFLAGS="$CPPFLAGS_BKUP"
else
 RTE_SDK=
 RTE_TARGET=
 RTE_LIBS=
 RTE_MBUF_HAS_PKT=
fi
AC_SUBST(RTE_SDK)
AC_SUBST(RTE_TARGET)
AC_SUBST(RTE_LIBS)
AC_SUBST(RTE_MBUF_HAS_PKT)

AC_ARG_ENABLE([mbtree],
	[AC_HELP_STRING([--enable-mbtree],
         [use multiple branch tree lookup [default is no]])],
	 [mbtree="$enableval"], [mbtree="no"])

MBTREE_FLAGS=''
if test "X${mbtree}" != "Xno"; then
   MBTREE_FLAGS="-DUSE_MBTREE"
   AC_SUBST(MBTREE_FLAGS)
fi

SSE42_CPPFLAGS=''
AC_ARG_ENABLE([sse4.2],
	[AC_HELP_STRING([--enable-sse4.2],
	 [build with SSE4.2 instruction [default is yes]])],
	 [sse42="$enableval"], [sse42="yes"])
if test "X${sse42}" != "Xno"; then
  SSE42_CPPFLAGS="-msse4.2"
fi
AC_SUBST(SSE42_CPPFLAGS)

# size_t printf() designator.
oCFLAGS=${CFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror"
AC_MSG_CHECKING([print format for size_t])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]],
		[[char buf[24];]
		 [snprintf(buf, sizeof(buf), "%zu", (size_t)1);]
		 [return (strcmp("1", buf) == 0 && strlen(buf) == 1) ? ]
		 [0 : 1;]])],
	[AC_DEFINE(HAVE_PRINT_FORMAT_FOR_SIZE_T)
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}

# pthread_setname_np()
oCFLAGS=${CFLAGS}
oLDFLAGS=${LDFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pthread"
LDFLAGS="${LDFLAGS} -lpthread"
AC_MSG_CHECKING([pthread_setname_np()])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <pthread.h>]],
		[[char buf[16];]
		 [snprintf(buf, sizeof(buf), "a thread");]
		 [return pthread_setname_np(pthread_self(), buf);]])],
	[AC_DEFINE(HAVE_PTHREAD_SETNAME_NP)
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}
LDFLAGS="${oLDFLAGS} ${LIBS}"

# pthread_setaffinity_np()
oCFLAGS=${CFLAGS}
oLDFLAGS=${LDFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pthread"
LDFLAGS="${LDFLAGS} -lpthread"
AC_MSG_CHECKING([pthread_setaffinity_np()])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <sched.h>]
		 [#include <pthread.h>]],
		[[cpu_set_t c;]
		 [CPU_ZERO(&c);]
		 [CPU_SET(0, &c);]
		 [return pthread_setaffinity_np(pthread_self(), sizeof(c), &c);]])],
	[AC_DEFINE(HAVE_PTHREAD_SETAFFINITY_NP)
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}
LDFLAGS="${oLDFLAGS} ${LIBS}"

# numa
AC_CHECK_HEADER(numa.h, [AC_DEFINE(HAVE_NUMA_H)])
AC_CHECK_HEADER(numaif.h, [AC_DEFINE(HAVE_NUMAIF_H)])

numa_supported=no
oCFLAGS=${CFLAGS}
oLDFLAGS=${LDFLAGS}
oLIBS=${LIBS}
CFLAGS="${CFLAGS} -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pthread"
LDFLAGS="${LDFLAGS} -lpthread"
LIBS="${LIBS} -lnuma"
AC_MSG_CHECKING([NUMA support])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <stdlib.h>]
		 [#include <string.h>]
		 [#include <numa.h>]
		 [#include <numaif.h>]],
		[[int node;]
		 [struct bitmask *bmp;]
		 [node = numa_node_of_cpu(0);]
		 [bmp = numa_allocate_nodemask();]
		 [numa_bitmask_setbit(bmp, node);]
		 [(void)set_mempolicy(MPOL_BIND, bmp->maskp, bmp->size + 1);]
		 [(void)numa_alloc_onnode(1024, node);]
		 [return 0;]])],
	[numa_supported=yes
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}
LDFLAGS=${oLDFLAGS}
LIBS=${oLIBS}

NUMA_LIBS=''
do_numa=${numa_supported}
AC_ARG_ENABLE([numa],
	[AC_HELP_STRING([--enable-numa],
	 [Support NUMA (if supported).])],
	[do_numa=${enableval}], [do_numa=${numa_supported}])
if test "X${do_numa}" = "Xyes"; then
  if test "X${numa_supported}" != "Xyes"; then
    AC_MSG_ERROR([The NUMA support library is not usable.])
    exit 1
  else
    AC_DEFINE(HAVE_NUMA)
    NUMA_LIBS='-lnuma'
    LIBS="${LIBS} ${NUMA_LIBS}"
    OS_LIBS="${OS_LIBS} ${NUMA_LIBS}"
  fi
fi
AC_SUBST(NUMA_LIBS)

# packaging
PKG_CONFFILE="mk/pkg_param.conf"
if test -f "${PKG_CONFFILE}"; then
   . "${PKG_CONFFILE}"
else
   AC_MSG_ERROR([NOT FOUND : mk/pkg_param.conf])
fi

AC_SUBST(PKG_NAME)
AC_SUBST(PKG_VERSION)
AC_SUBST(PKG_REVISION)
AC_SUBST(PKG_MAINTAINER)
AC_SUBST(PKG_SECTION)
AC_SUBST(PKG_PRIORITY)
AC_SUBST(PKG_DESCRIPTION_RAW_SOCKET)
AC_SUBST(PKG_DESCRIPTION_DPDK)
AC_SUBST(PKG_HP_URL)
AC_SUBST(PKG_VCS)
AC_SUBST(PKG_VCS_URL)
AC_SUBST(PKG_REMOVE_DIRS)
AC_SUBST(PKG_REMOVE_FILES)
AC_SUBST(PKG_PURGE_DIRS)
AC_SUBST(PKG_PURGE_FILES)

sh mk/cat-deb-control.sh ${RTE_SDK}
if test "x$?" != "x0"; then
   AC_MSG_ERROR([NOT FOUND : debian/control_*.])
fi

# emit.
AC_CONFIG_FILES(
	mk/vars.mk
	mk/doxygen.conf
	Makefile
	src/include/Makefile
	src/include/lagopus_platform.h
	src/lib/Makefile
	src/lib/test/Makefile
	src/lib/check/Makefile
	src/agent/Makefile
	src/agent/test/Makefile
	src/cmds/Makefile
	src/config/lagosh/Makefile
	src/config/lagosh/lagosh.py
	src/dataplane/Makefile
	src/dataplane/dpdk/test/Makefile
	src/dataplane/ofproto/test/Makefile
	src/dataplane/ofproto/test/lib/Makefile
	src/dataplane/mgr/test/Makefile
	src/datastore/Makefile
	src/datastore/test/Makefile
	src/datastore/check/Makefile
	src/snmp/Makefile
	src/snmp/test/Makefile
	test/dataplane/benchmark/Makefile
	tools/Makefile
	tools/unity/src/Makefile
	debian/Makefile
	debian/control
	debian/copyright
	debian/lagopus-DATAPLANE.install,
)

# emit and set executable attribute.
AC_DEFUN([CONFIG_EXECS],
[
m4_foreach_w(
    [FILE_NAME],
    [$1],
    [AC_CONFIG_FILES(FILE_NAME, chmod +x FILE_NAME)])
])

CONFIG_EXECS(
	debian/rules
	debian/lagopus-DATAPLANE.postrm
)

AC_CONFIG_COMMANDS([modify-config],
	[sh mk/finalizeconfig ${OS_CPPFLAGS}],
	[OS_CPPFLAGS="${OS_CPPFLAGS}"])

AC_CONFIG_COMMANDS([depend],
	[sh mk/initdepend && ${MAKE} depend],
	[MAKE=${MAKE}])

AC_CONFIG_COMMANDS([config-clean],
	[rm -rf ./config.log ./config.status.* autom4te.cache])

AC_OUTPUT

${MAKE} clean
${MAKE} generate

echo ""
echo "CC:	'${CC}'"
echo "CFLAGS:	'${OS_CFLAGS}'"
echo "CPPFLAGS:	'${CPPFLAGS} ${OS_CPPFLAGS} ${CPUDEF} ${OSDEF}'"
echo "LDFLAGS:	'${LDFLAGS} ${OS_LDFLAGS} ${OS_LIBS}'"
echo ""
