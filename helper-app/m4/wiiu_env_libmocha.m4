# -*- mode: autoconf -*-
# wiiu_env_libmocha.m4 - Macros to handle libmocha
# URL: https://github.com/dkosmari/devkitpro-autoconf/

# Copyright (c) 2026 Daniel K. O. <dkosmari>
#
# Copying and distribution of this file, with or without modification, are permitted in
# any medium without royalty provided the copyright notice and this notice are
# preserved. This file is offered as-is, without any warranty.

#serial 1

# WIIU_ENV_SETUP_LIBMOCHA([RELATIVE-PATH-TO-LIBMOCHA])
# ----------------------------------------------------
#
# Sets up usage of libmocha as a submodule.
#
# Output variables:
#   - `LIBMOCHA_DIR'
#   - `LIBMOCHA_CPPFLAGS'
#   - `LIBMOCHA_LIB'
#   - custom Makefile rules (use @INC_AMINCLUDE@ in your `Makefile.am')

AC_DEFUN([WIIU_ENV_SETUP_LIBMOCHA],[dnl
    WIIU_ENV_SETUP_LIB([LIBMOCHA], [], [$1], [${DEVKITPRO}/wut/usr])
])dnl WIIU_ENV_SETUP_LIBMOCHA
