# -*- mode: autoconf -*-
# devkitpro_wut.m4 - Macros to handle WUT setup.
# URL: https://github.com/dkosmari/devkitpro-autoconf/

# Copyright (c) 2025-2026 Daniel K. O. <dkosmari>
#
# Copying and distribution of this file, with or without modification, are permitted in
# any medium without royalty provided the copyright notice and this notice are
# preserved. This file is offered as-is, without any warranty.

#serial 8

# DEVKITPRO_WUT_INIT
# ------------------
#
# This macro adjusts paths for Wii U homebrew, using WUT.
# Call this before `AM_INIT_AUTOMAKE`. It will call `DEVKITPRO_PPC_INIT'.
#
# Output variables:
#   - `ELF2RPL': set to `elf2rpl' binary.
#   - `PATH': appends tools and portlibs paths.
#   - `WUHBTOOL': set to `wuhbtool' binary.
#   - `WUT_ROOT': set to `DEVKITPRO/wut'

AC_DEFUN([DEVKITPRO_WUT_INIT],[

    DEVKITPRO_PPC_INIT

    # Ensure ${DEVKITPRO}/tools/bin is in PATH
    DEVKITPRO_APPEND_TOOL_PATH([elf2rpl], [${DEVKITPRO}/tools/bin])

    AC_CHECK_PROGS([ELF2RPL], [elf2rpl], [], [${PATH}:${DEVKITPRO_PATH}])
    AC_CHECK_PROGS([WUHBTOOL], [wuhbtool], [], [${PATH}:${DEVKITPRO_PATH}])

    # set DEVKITPRO_PORTLIBS_WIIU
    AC_ARG_VAR([DEVKITPRO_PORTLIBS_WIIU], [path to portlibs/wiiu])
    AS_VAR_SET_IF([DEVKITPRO_PORTLIBS_WIIU],
                  [],
                  [AS_VAR_SET([DEVKITPRO_PORTLIBS_WIIU], [${DEVKITPRO_PORTLIBS}/wiiu])])
    AC_SUBST([DEVKITPRO_PORTLIBS_WIIU])

    # Prepend portlibs/wiiu/bin and portlibs/ppc/bin to DEVKITPRO_PATH
    # Note: we don't know if any portlibs package is installed or even needed.
    # Note: wiiu appears before ppc.
    DEVKITPRO_PATH_PREPEND([${DEVKITPRO_PORTLIBS_PPC}/bin])
    DEVKITPRO_PATH_PREPEND([${DEVKITPRO_PORTLIBS_WIIU}/bin])

    # set WUT_ROOT
    AC_ARG_VAR([WUT_ROOT], [path to wut])
    AS_VAR_SET_IF([WUT_ROOT],
                  [],
                  [AS_VAR_SET([WUT_ROOT], [${DEVKITPRO}/wut])])
    
    AC_SUBST([WUT_ROOT])

    # Finally, append DEVKITPRO_PATH to PATH if needed
    AS_VAR_SET_IF([DEVKITPRO_PATH],
                  [AS_VAR_APPEND([PATH], [":${DEVKITPRO_PATH}"])])

])


# DEVKITPRO_WUT_OPT_INIT
# ----------------------
#
# Calls DEVKITPRO_WUT_INIT only if `--enable-wiiu' argument is given.

AC_DEFUN([DEVKITPRO_WUT_OPT_INIT],[

    AC_ARG_ENABLE([wiiu],
                  [AS_HELP_STRING([--enable-wiiu], [build Wii U homebrew])])

    AS_VAR_IF([enable_wiiu], [yes], [DEVKITPRO_WUT_INIT])

])dnl DEVKITPRO_WUT_OPT_INIT


# DEVKITPRO_WUT_SETUP
# -------------------
#
# This macro adjusts compilation flags for Wii U homebrew, using WUT.
#
# Output variables:
#   - `CFLAGS'
#   - `CPPFLAGS'
#   - `CXXFLAGS'
#   - `LDFLAGS'
#   - `LIBS'

AC_DEFUN([DEVKITPRO_WUT_SETUP],[

    AS_VAR_SET_IF([WUT_ROOT], [], [AC_MSG_ERROR([WUT_ROOT not set.])])

    DEVKITPRO_PPC_SETUP

    AX_PREPEND_FLAG([-D__WIIU__],                            [CPPFLAGS])
    AX_PREPEND_FLAG([-D__wiiu__],                            [CPPFLAGS])
    AX_PREPEND_FLAG([-D__WUT__],                             [CPPFLAGS])
    AX_PREPEND_FLAG([-I${DEVKITPRO_PORTLIBS_WIIU}/include],  [CPPFLAGS])
    AX_PREPEND_FLAG([-I${WUT_ROOT}/usr/include],             [CPPFLAGS])
    AX_PREPEND_FLAG([-I${WUT_ROOT}/include],                 [CPPFLAGS])

    AX_PREPEND_FLAG([-mcpu=750],    [CFLAGS])
    AX_PREPEND_FLAG([-meabi],       [CFLAGS])
    AX_PREPEND_FLAG([-mhard-float], [CFLAGS])

    AX_PREPEND_FLAG([-mcpu=750],    [CXXFLAGS])
    AX_PREPEND_FLAG([-meabi],       [CXXFLAGS])
    AX_PREPEND_FLAG([-mhard-float], [CXXFLAGS])

    AX_PREPEND_FLAG([-L${WUT_ROOT}/lib],                [LIBS])
    AX_PREPEND_FLAG([-L${WUT_ROOT}/usr/lib],            [LIBS])
    AX_PREPEND_FLAG([-L${DEVKITPRO_PORTLIBS_WIIU}/lib], [LIBS])

    AX_PREPEND_FLAG([-specs=${WUT_ROOT}/share/wut.specs], [LDFLAGS])

    DEVKITPRO_CHECK_LIBRARY([wut.h],
                            [wut],
                            [],
                            [],
                            [AX_APPEND_FLAG([-lwut], [LIBS])],
                            [AC_MSG_ERROR([wut not found in ${DEVKITPRO}; install the package with "dkp-pacman -S wut"])])

    m4_pattern_allow([AM_V_at])

    # custom Makefile recipes for building RPX
    AX_ADD_AM_MACRO([

.PHONY: clean-rpx

clean: clean-rpx

clean-rpx:
	\$([AM_V_at])\$(RM) *.rpx

%.rpx: %.strip.elf
	\$([AM_V_at])\$(ELF2RPL) \$< \$[@]

])

])dnl DEVKITPRO_WUT_SETUP


AC_DEFUN([DEVKITPRO_WUT_SETUP_RPL],[

    AS_VAR_SET_IF([WUT_ROOT], [], [AC_MSG_ERROR([WUT_ROOT not defined.])])

    AX_PREPEND_FLAG([-specs=${WUT_ROOT}/share/rpl.specs], [LDFLAGS])

    m4_pattern_allow([AM_V_at])

    # custom Makefile recipes for building RPL
    AX_ADD_AM_MACRO([

.PHONY: clean-rpl

clean: clean-rpl

clean-rpl:
	\$([AM_V_at])\$(RM) *.rpl

%.rpl: %.strip.elf
	\$([AM_V_at])\$(ELF2RPL) --rpl \$< \$[@]

])

])dnl DEVKITPRO_WUT_SETUP_RPL
