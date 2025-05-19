# -*- mode: autoconf -*-
# devkitpro.m4 - Macros to handle devkitPro setup.
# URL: https://github.com/dkosmari/devkitpro-autoconf/

# Copyright (c) 2025 Daniel K. O. <dkosmari>
#
# Copying and distribution of this file, with or without modification, are permitted in
# any medium without royalty provided the copyright notice and this notice are
# preserved. This file is offered as-is, without any warranty.

#serial 4

# DEVKITPRO_INIT
# --------------
#
# This macro sets up base devkitPro variables to be used by other macros. The option
# `--with-devkitpro=' is also processed, to override the `DEVKITPRO' variable.
#
# Output variables:
#   - `DEVKITPRO': path to devkitPro.
#   - `DEVKITPRO_PORTLIBS': path to portlibs.
#
# The file `aminclude.am` is generated with extra Makefile rules:
#   - Add `@INC_AMINCLUDE@` to the Makefile that needs them.
#   - Add `DISTCLEANFILES = $(AMINCLUDE)' to the toplevel `Makefile.am` to remove this
#     file during `make distclean'.

AC_DEFUN([DEVKITPRO_INIT], [

    AC_REQUIRE([AC_CANONICAL_HOST])

    # Make sure macros that look up programs don't appear before this, since we may need
    # to adjust PATH.
    AC_BEFORE([$0], [AM_INIT_AUTOMAKE])
    # specific program tests
    AC_BEFORE([$0], [AC_PROG_CC])
    AC_BEFORE([$0], [AC_PROG_CXX])
    AC_BEFORE([$0], [AC_PROG_CPP])
    AC_BEFORE([$0], [AC_PROG_RANLIB])
    # automake also has these
    AC_BEFORE([$0], [AM_PROG_AR])
    AC_BEFORE([$0], [AM_PROG_AS])
    # cross-compilation tool tests
    AC_BEFORE([$0], [AC_CHECK_TOOL])
    AC_BEFORE([$0], [AC_CHECK_TOOLS])
    AC_BEFORE([$0], [AC_PATH_TARGET_TOOL])
    AC_BEFORE([$0], [AC_PATH_TOOL])
    AC_BEFORE([$0], [PKG_PROG_PKG_CONFIG])

    AC_ARG_VAR([DEVKITPRO], [path to devkitPro])

    # --with-devkitpro can override DEVKITPRO variable
    AC_ARG_WITH([devkitpro],
                [AS_HELP_STRING([--with-devkitpro=PATH-TO-DEVKITPRO],
                                [Set the base path to devkitPro. This overrides the variable DEVKITPRO])],
                [AS_VAR_SET([DEVKITPRO], [$withval])])

    # check if DEVKITPRO is set
    AC_MSG_CHECKING([devkitPro path])

    AS_VAR_SET_IF([DEVKITPRO],
                  [
                      AC_MSG_RESULT([$DEVKITPRO])
                  ],
                  [
                      AC_MSG_RESULT([not found])
                      AC_MSG_ERROR([need --with-devkitpro=PATH-TO-DEVKITPRO or DEVKITPRO=PATH-TO-DEVKITPRO])
                  ])

    AC_SUBST([DEVKITPRO])

    # set DEVKITPRO_PORTLIBS
    AC_ARG_VAR([DEVKITPRO_PORTLIBS], [path to portlibs])
    AS_VAR_SET([DEVKITPRO_PORTLIBS], [$DEVKITPRO/portlibs])
    AC_SUBST([DEVKITPRO_PORTLIBS])

    # custom Makefile recipes
    AX_ADD_AM_MACRO([
clean: clean-strip-elf
.PHONY: clean-strip-elf
clean-strip-elf:; \$(RM) *.strip.elf
%.strip.elf: %.elf; \$(STRIP) -g \$< -o \$[@]
])

    # make updated PATH visible to Makefiles
    AC_ARG_VAR([PATH])
    AC_SUBST([PATH])

])dnl DEVKITPRO_INIT


# DEVKITPRO_APPEND_PATH(SEARCH-PATH)
# ----------------------------------
#
# Appends SEARCH-PATH to PATH if it's not there already.
#
# Output variables:
#   - `PATH'

AC_DEFUN([DEVKITPRO_APPEND_PATH], [

    if @<:@@<:@ ":@S|@PATH:" != *":$1:"* @:>@@:>@
    then
          AS_VAR_APPEND([PATH], [":$1"])
    else
          AC_MSG_RESULT([yes])
    fi

])dnl DEVKITPRO_APPEND_PATH


# DEVKITPRO_APPEND_TOOL_PATH(EXECUTABLE, SEARCH-PATH)
# ---------------------------------------------------
#
# This macro checks if an executable is found in PATH; if not, appends SEARCH-PATH to PATH.
#
# Output variables:
#   - `PATH'

AC_DEFUN([DEVKITPRO_APPEND_TOOL_PATH], [
    AC_MSG_CHECKING([if $2 is in PATH])
    AS_IF([! which $1 1>/dev/null 2>/dev/null],
          [
              AC_MSG_RESULT([no, will append $2 to PATH])
              DEVKITPRO_APPEND_PATH([$2])
              # Test if it's usable.
              AS_IF([! which $1 1>/dev/null 2>/dev/null],
                    [AC_MSG_ERROR([could not execute $1])])
          ],
          [AC_MSG_RESULT([yes])])
])dnl DEVKITPRO_APPEND_TOOL_PATH


# DEVKITPRO_CHECK_LIBRARY(HEADER-FILE,
#                         LIBRARY-FILE,
#                         CPPFLAGS,
#                         LDFLAGS,
#                         ACTION-IF-FOUND,
#                         ACTION-IF-NOT-FOUND)
# --------------------------------------------
#
# Similar to AX_CHECK_LIBRARY, but does not create special variables.

AC_DEFUN([DEVKITPRO_CHECK_LIBRARY], [

    AX_VAR_PUSHVALUE([CPPFLAGS], [$CPPFLAGS $3])
    AX_VAR_PUSHVALUE([LDFLAGS], [$LDFLAGS $4])

    AC_CHECK_HEADER([$1],
                    [
                        AC_CHECK_LIB([$2],
                                     [main],
                                     [_FOUND_LIB=yes],
                                     [_FOUND_LIB=no])
                    ],
                    [_FOUND_LIB=no])

    AX_VAR_POPVALUE([LDFLAGS])
    AX_VAR_POPVALUE([CPPFLAGS])

    AS_VAR_IF([_FOUND_LIB], [yes],
              [$5],
              [$6])

    AS_UNSET([_FOUND_LIB])

])dnl DEVKITPRO_CHECK_LIBRARY


# DEVKITPRO_CHECK_LIBRARY_FULL(PREFIX,
#                              HEADER-FILE,
#                              LIBRARY-FILE,
#                              CPPFLAGS,
#                              LDFLAGS,
#                              ACTION-IF-FOUND,
#                              ACTION-IF-NOT-FOUND)
#--------------------------------------------------
#
# Similar to AX_CHECK_LIBRARY, but generates _CPPFLAGS, _LDFLAGS and _LIBS

AC_DEFUN([DEVKITPRO_CHECK_LIBRARY_FULL], [

    AC_ARG_VAR($1[_CPPFLAGS], [C preprocessor flags for ]$1[ headers])
    AC_ARG_VAR($1[_LDFLAGS], [linker flags for ]$1[ libraries])
    AC_ARG_VAR($1[_LIBS], [library link option for ]$1[ libraries])

    AC_CACHE_VAL(AS_TR_SH([dkp_cv_have_]$1),
                 [
                     AX_VAR_PUSHVALUE([CPPFLAGS], [$CPPFLAGS $4])
                     AX_VAR_PUSHVALUE([LDFLAGS], [$LDFLAGS $5])

                     AC_CHECK_HEADER($2,
                                     [
                                         AC_CHECK_LIB($3,
                                                      [main],
                                                      [AS_TR_SH([dkp_cv_have_]$1)=yes],
                                                      [AS_TR_SH([dkp_cv_have_]$1)=no])
                                     ],
                                     [AS_TR_SH([dkp_cv_have_]$1)=no])

                     AX_VAR_POPVALUE([LDFLAGS])
                     AX_VAR_POPVALUE([CPPFLAGS])
                 ])

    AS_IF([test "$]AS_TR_SH([dkp_cv_have_]$1)[" = "yes"],
          [
              AC_DEFINE([HAVE_]$1, [1], [Define to 1 if ]$1[ is found])
              AS_VAR_SET($1[_LIBS], ["-l$3"])
              AS_VAR_SET($1[_CPPFLAGS], ["$4"])
              AS_VAR_SET($1[_LDFLAGS], ["$5"])
              [$6]
          ],
          [$7])

])dnl DEVKITPRO_CHECK_LIBRARY_FULL


