#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([System Font Replacer],
        [1.3],
        [https://github.com/dkosmari/System-Font-Replacer/issues],
        [],
        [https://github.com/dkosmari/System-Font-Replacer])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_HOST

DEVKITPRO_WUT_INIT

AM_INIT_AUTOMAKE([foreign subdir-objects tar-pax])

LT_INIT([disable-shared])

AC_PROG_CXX
AX_APPEND_COMPILE_FLAGS([-std=c++23], [CXX])
AC_LANG([C++])

DEVKITPRO_WUT_SETUP
WIIU_ENV_SETUP_WUPS([external/libwupsxx/external/WiiUPluginSystem])

AX_SUBDIRS_CONFIGURE([external/libwupsxx])
AX_SUBDIRS_CONFIGURE([helper-app])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
