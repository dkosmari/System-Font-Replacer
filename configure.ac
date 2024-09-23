#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([System Font Replacer],
        [1.1.3+],
        [https://github.com/dkosmari/System-Font-Replacer/issues],
        [],
        [https://github.com/dkosmari/System-Font-Replacer])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

WIIU_WUPS_INIT

AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_CXX
AX_APPEND_COMPILE_FLAGS([-std=c++23], [CXX])
AC_LANG([C++])

AC_CONFIG_FILES([Makefile])

AC_CONFIG_SUBDIRS([external/libwupsxx
                   helper-app])

AC_OUTPUT
