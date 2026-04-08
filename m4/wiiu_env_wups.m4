# -*- mode: autoconf -*-
# wiiu_env_wups.m4 - Macros to handle Wii U Plugin System
# URL: https://github.com/dkosmari/devkitpro-autoconf/

# Copyright (c) 2025-2026 Daniel K. O. <dkosmari>
#
# Copying and distribution of this file, with or without modification, are permitted in
# any medium without royalty provided the copyright notice and this notice are
# preserved. This file is offered as-is, without any warranty.

#serial 3

# WIIU_ENV_SETUP_WUPS([RELATIVE-PATH-TO-WUPS])
# --------------------------------------------
#
# This macro adjusts compilation flags for the Wii U Plugin System (WUPS).
#
# Output variables:
#   - `WUPS_DIR'
#   - `WUPS_CPPFLAGS'
#   - `WUPS_LDFLAGS'
#   - `WUPS_LIB'
#   - custom Makefile rules (use @INC_AMINCLUDE@ in your `Makefile.am')

AC_DEFUN([WIIU_ENV_SETUP_WUPS],[

    AC_REQUIRE([DEVKITPRO_WUT_SETUP])

    # Allow user to override WUPS_DIR while running ./configure
    AC_ARG_VAR([WUPS_DIR], [path to WUPS])
    AS_VAR_SET_IF([WUPS_DIR],
                  [AC_MSG_NOTICE([user override for WUPS_DIR="${WUPS_DIR}"])],
                  [AS_VAR_SET([WUPS_DIR], "m4_default([$1], [${DEVKITPRO}/wups])")])
    AC_SUBST([WUPS_DIR])

    AC_ARG_VAR([WUPS_CPPFLAGS], [preprocessor flags for WUPS])
    AS_VAR_SET([WUPS_CPPFLAGS], ["-D__WUPS__ -I${WUPS_DIR}/include"])
    AC_SUBST([WUPS_CPPFLAGS])

    AC_ARG_VAR([WUPS_LDFLAGS], [linker flags for WUPS])
    AS_VAR_SET([WUPS_LDFLAGS], ["-T${WUPS_DIR}/share/wups.ld -specs=${WUPS_DIR}/share/wups.specs"])
    AC_SUBST([WUPS_LDFLAGS])

    AC_ARG_VAR([WUPS_LIB], [relative path to the WUPS library])
    AS_VAR_SET([WUPS_LIB], ["${WUPS_DIR}/lib/libwups.a"])
    AC_SUBST([WUPS_LIB])

    AC_ARG_VAR([WUPS_DEBUG_LIB], [relative path to the WUPS debug library])
    AS_VAR_SET([WUPS_DEBUG_LIB], ["${WUPS_DIR}/lib/libwupsd.a"])
    AC_SUBST([WUPS_DEBUG_LIB])

    m4_pattern_allow([AM_V_at])

    # Makefile rules for building .wps plugins
    AX_ADD_AM_MACRO([

.PHONY: clean-wps

clean: clean-wps

clean-wps:
	\$([AM_V_at])\$(RM) *.wps

%.wps: %.strip.elf
	\$([AM_V_at])\$(ELF2RPL) \$< \$[@]
	\$([AM_V_at])printf "PL" | dd of=\$[@] bs=1 seek=9 count=2 conv=notrunc status=none

])

    # Makefile rule for building and cleaning WUPS
    AX_ADD_AM_MACRO([
$WUPS_LIB:
	-\$([AM_V_at])\$(MAKE) \$(AM_MAKEFLAGS) -C \$(WUPS_DIR) lib/libwups.a

$WUPS_DEBUG_LIB:
	-\$([AM_V_at])\$(MAKE) \$(AM_MAKEFLAGS) -C \$(WUPS_DIR) lib/libwupsd.a

.PHONY: clean-wiiu-env-WUPS

clean: clean-wiiu-env-WUPS

distclean: clean-wiiu-env-WUPS

clean-wiiu-env-WUPS:
	-\$([AM_V_at])\$(MAKE) \$(AM_MAKEFLAGS) -C \$(WUPS_DIR) clean

])

])dnl WIIU_ENV_SETUP_WUPS
