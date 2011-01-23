dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([aped], [2.0], [a.catel@weelya.com])
AC_CANONICAL_SYSTEM

AC_CONFIG_SRCDIR([src/core/ape_entry.c])
AM_INIT_AUTOMAKE([foreign -Wall])

AM_MAINTAINER_MODE
AC_LANG(C)
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])
#AC_CHECK_HEADERS([sys/epoll.h])
AC_CHECK_FUNCS([epoll_ctl])
AC_CONFIG_FILES([Makefile src/core/Makefile])

# --enable-kqueue
AC_ARG_ENABLE(kqueue,
    AS_HELP_STRING([--enable-kqueue],
        [use kqueue if available (default is YES)]),
    ,
    [enable_kqueue=yes])
if test "$enable_kqueue" = yes; then
        case $target in
        *-*-freebsd* | *-*-darwin* )
                AC_CHECK_FUNCS([kqueue])
                ;;
        *-*-bsd*)
                # No other BSD has a sufficiently recent implementation
                AC_MSG_WARN([wont look for kqueue() on $target])
                ac_cv_func_kqueue=no
                ;;
        esac
else
        ac_cv_func_kqueue=no
fi
#AC_CONFIG_SUBDIRS([deps/confuse-2.7 deps/c-ares/])

AC_OUTPUT
