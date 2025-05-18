# -*- mode: autoconf -*-
# wiiu_wups.m4 - Macros to handle Wii U Plugin System
# URL: https://github.com/dkosmari/devkitpro-autoconf/

# Copyright (c) 2025 Daniel K. O. <dkosmari>
#
# Copying and distribution of this file, with or without modification, are permitted in
# any medium without royalty provided the copyright notice and this notice are
# preserved. This file is offered as-is, without any warranty.

#serial 4

# WIIU_WUPS_INIT
# --------------
# This macro adjusts the environment for the Wii U Plugin System (WUPS).
#
# Output variables:
#   - `WIIU_WUPS'

AC_DEFUN([WIIU_WUPS_INIT],[

    AC_REQUIRE([DEVKITPRO_WUT_INIT])

    # set WIIU_WUPS
    AS_VAR_SET([WIIU_WUPS], [$DEVKITPRO/wups])

])dnl WIIU_WUPS_INIT


# WIIU_WUPS_SETUP
# ---------------
# This macro adjusts compilation flags for the Wii U Plugin System (WUPS).
#
# Output variables:
#   - `CPPFLAGS'
#   - `LDFLAGS'
#   - `LIBS'

AC_DEFUN([WIIU_WUPS_SETUP],[

    AC_REQUIRE([DEVKITPRO_WUT_SETUP])

    AS_VAR_SET_IF([WIIU_WUPS], [], [AC_MSG_ERROR([WIIU_WUPS not set])])

    AX_PREPEND_FLAG([-D__WUPS__],           [CPPFLAGS])
    AX_PREPEND_FLAG([-I$WIIU_WUPS/include], [CPPFLAGS])

    AX_PREPEND_FLAG([-T$WIIU_WUPS/share/wups.ld],         [LDFLAGS])
    AX_PREPEND_FLAG([-specs=$WIIU_WUPS/share/wups.specs], [LDFLAGS])

    AX_PREPEND_FLAG([-L$WIIU_WUPS/lib], [LIBS])

    # check for header and lib
    AX_CHECK_LIBRARY([WIIU_WUPS_LIBWUPS],
                     [wups.h],
                     [wups],
                     [AX_PREPEND_FLAG([-lwups], [LIBS])],
                     [AC_MSG_ERROR([WUPS not found; get it from https://github.com/wiiu-env/WiiUPluginSystem])])


    # custom Makefile rules for .wps plugins
    AX_ADD_AM_MACRO([
clean: clean-wps
.PHONY: clean-wps
clean-wps:; \$(RM) *.wps
%.wps: %.strip.elf
	\$(ELF2RPL) \$< \$[@]
	printf 'PL' | dd of=\$[@] bs=1 seek=9 count=2 conv=notrunc status=none
])

])dnl WIIU_WUPS_SETUP
