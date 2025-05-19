# -*- mode: autoconf -*-
# devkitpro_ppc.m4 - Macros to handle PPC toolchains.
# URL: https://github.com/dkosmari/devkitpro-autoconf/

# Copyright (c) 2025 Daniel K. O. <dkosmari>
#
# Copying and distribution of this file, with or without modification, are permitted in
# any medium without royalty provided the copyright notice and this notice are
# preserved. This file is offered as-is, without any warranty.

#serial 3

# DEVKITPRO_PPC_INIT
# ------------------
#
# This macro adjuts the environment for PPC-based targets. It must be called before
# `AM_INIT_AUTOMAKE', and before any cross-compilation tool is checked.
#
# Output variables:
#   - `DEVKITPPC': sets path to devkitPPC
#   - `PATH': appends `:$DEVKITPPC/bin' if necessary.
#   - `DEVKITPRO_PORTLIBS_PPC': sets path to portlibs/ppc

AC_DEFUN([DEVKITPRO_PPC_INIT],[

    DEVKITPRO_INIT

    # Sanity check for host type.
    AS_CASE($host,
            [powerpc-*-eabi], [],
            [AC_MSG_ERROR([invalid host ($host), you should use --host=powerpc-eabi])])


    # set DEVKITPPC
    AC_ARG_VAR([DEVKITPPC], [path to devkitPPC])
    # if not set, set it to $DEVKITPRO/devkitPPC
    AS_VAR_SET_IF([[DEVKITPPC]],
                  [],
                  [AS_VAR_SET([DEVKITPPC], [$DEVKITPRO/devkitPPC])])
    AC_SUBST([DEVKITPPC])

    # See if we can find cross tools in PATH already; if not, append $DEVKITPPC/bin to
    # PATH
    DEVKITPRO_APPEND_TOOL_PATH([powerpc-eabi-nm], [$DEVKITPPC/bin])

    # Now check that DEVKITPPC/bin binaries are usable
    AS_IF([! which powerpc-eabi-nm 1>/dev/null 2>/dev/null],
          [AC_MSG_ERROR([devkitPPC binaries not found in PATH=$PATH])])

    AC_ARG_VAR([DEVKITPRO_PORTLIBS_PPC], [path to portlibs/ppc])
    AS_VAR_SET([DEVKITPRO_PORTLIBS_PPC], [$DEVKITPRO_PORTLIBS/ppc])
    AC_SUBST([DEVKITPRO_PORTLIBS_PPC])

])dnl DEVKITPRO_PPC_INIT


# DEVKITPRO_PPC_SETUP
# -------------------
#
# This macro adjuts CPPFLAGS and LIBS to use portlibs libraries.
# Call this after automake and tools check, to allow automake to set default
# CFLAGS/CXXFLAGS.
#
# Output variables:
#   - `CPPFLAGS': prepends search path for portlibs/ppc/include
#   - `LIBS': prepends search path for portlibs/ppc/lib

AC_DEFUN([DEVKITPRO_PPC_SETUP],[

    AS_VAR_SET_IF([DEVKITPRO_PORTLIBS_PPC],
                  [],
                  [AC_MSG_ERROR([DEVKITPRO_PORTLIBS_PPC not defined!])])

    AX_PREPEND_FLAG([-I$DEVKITPRO_PORTLIBS_PPC/include], [CPPFLAGS])

    AX_PREPEND_FLAG([-L$DEVKITPRO_PORTLIBS_PPC/lib], [LIBS])

])dnl DEVKITPRO_PPC_SETUP
