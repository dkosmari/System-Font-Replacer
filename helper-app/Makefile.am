EXTRA_DIST = \
	assets \
	AUTHORS \
	bootstrap \
	COPYING \
	README.md


SUBDIRS = external/libmocha

DIST_SUBDIRS =


AM_CXXFLAGS = -Wall -Wextra -Werror

AM_CPPFLAGS = $(LIBMOCHA_CPPFLAGS)

LDADD = $(LIBMOCHA_LIB)

ELF_FILE = system-font-replacer-helper.elf

RPX_FILE  = $(ELF_FILE:.elf=.rpx)

WUHB_FILE = $(ELF_FILE:.elf=.wuhb)

CLEANFILES = $(WUHB_FILE) $(RPX_FILE)


noinst_PROGRAMS = $(ELF_FILE)

system_font_replacer_helper_elf_SOURCES = \
	src/bps.cpp \
	src/bps.hpp \
	src/crc32.cpp \
	src/crc32.hpp \
	src/glue_stdio.cpp \
	src/main.cpp \
	src/mocha.cpp \
	src/mocha.hpp \
	src/whb.cpp src/whb.hpp

EXTRA_system_font_replacer_helper_elf_DEPENDENCIES = $(LIBMOCHA_LIB)



$(WUHB_FILE): $(RPX_FILE)
	$(WUHBTOOL) $< $@ \
		--icon=assets/icon.png \
		--author="Daniel K. O." \
		--name="$(PACKAGE_NAME)" \
		--short-name="$(PACKAGE_TARNAME)-helper"


all-local: $(WUHB_FILE)


.PHONY: run wiiu-install wiiu-uninstall company


run: $(WUHB_FILE)
	WIILOAD=tcp:wiiu wiiload $<


wiiu-install: $(WUHB_FILE)
	curl "ftp://wiiu:/fs/vol/external01/wiiu/apps/" --upload-file $<


wiiu-uninstall:
	curl "ftp://wiiu" --quote "DELE /fs/vol/external01/wiiu/apps/$(WUHB_FILE)"


company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> compile_flags.txt


@INC_AMINCLUDE@
DISTCLEANFILES = $(AMINCLUDE)

