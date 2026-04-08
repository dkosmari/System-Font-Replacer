# -*- mode: autoconf -*-
# wiiu_env_setup_lib.m4 - Macros to handle libraries as submodules.
# URL: https://github.com/dkosmari/devkitpro-autoconf/

# Copyright (c) 2026 Daniel K. O. <dkosmari>
#
# Copying and distribution of this file, with or without modification, are permitted in
# any medium without royalty provided the copyright notice and this notice are
# preserved. This file is offered as-is, without any warranty.

#serial 3

# WIIU_ENV_SETUP_LIB(PREFIX, [LIB-FILE], [LIB-DIR], SYSTEM-LIB-DIR)
# -----------------------------------------------------------------
#
# Sets up usage of a library as a submodule.
#
# Output variables:
#   - `PREFIX_DIR'
#   - `PREFIX_CPPFLAGS'
#   - `PREFIX_LIB'
#   - custom Makefile rules (use @INC_AMINCLUDE@ in your `Makefile.am')

AC_DEFUN([WIIU_ENV_SETUP_LIB],[

    AC_ARG_VAR([$1_DIR], [path to $1])
    # Allow user to override $1_DIR while running ./configure
    AS_VAR_SET_IF([$1_DIR],
                  [AC_MSG_NOTICE([user override for $1_DIR="${$1_DIR}"])],
                  [AS_VAR_SET([$1_DIR],["m4_default([$3],[$4])"])])
    AC_SUBST([$1_DIR])

    AC_ARG_VAR([$1_CPPFLAGS], [preprocessor flags for $1])
    AS_VAR_SET([$1_CPPFLAGS], ["-I${$1_DIR}/include"])
    AC_SUBST([$1_CPPFLAGS])

    AC_ARG_VAR([$1_LIB], [relative path to the $1 library])
    AS_VAR_SET([$1_LIB], ["${$1_DIR}/m4_default([$2],[lib/m4_tolower([$1]).a])"])
    AC_SUBST([$1_LIB])

    m4_pattern_allow([AM_V_at])

    # Makefile rules for building and cleaning $1
    AX_ADD_AM_MACRO([

.PHONY: clean-wiiu-env-$1

clean: clean-wiiu-env-$1

distclean: clean-wiiu-env-$1

clean-wiiu-env-$1:
	-\$([AM_V_at])\$(MAKE) \$(AM_MAKEFLAGS) -C \$($1_DIR) clean V=\$V

${$1_LIB}:
	-\$([AM_V_at])\$(MAKE) \$(AM_MAKEFLAGS) -C \$($1_DIR) V=\$V

])

])

