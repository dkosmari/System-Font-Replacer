@INC_AMINCLUDE@
DISTCLEANFILES = $(AMINCLUDE)


EXTRA_DIST = \
	assets \
	AUTHORS \
	bootstrap \
	COPYING \
	README.md


AM_CPPFLAGS = $(DEVKITPRO_CPPFLAGS)

AM_CXXFLAGS = \
	-Wall -Wextra \
	$(DEVKITPRO_CXXFLAGS)

AM_LDFLAGS = $(DEVKITPRO_LDFLAGS)

LIBS = $(DEVKITPRO_LIBS)


RPX_FILE  = system-font-replacer-helper.rpx
WUHB_FILE = system-font-replacer-helper.wuhb
CLEANFILES = $(WUHB_FILE) $(RPX_FILE)


noinst_PROGRAMS = system-font-replacer-helper.elf

system_font_replacer_helper_elf_SOURCES = \
	src/bps.cpp src/bps.hpp \
	src/crc32.cpp src/crc32.hpp \
	src/main.cpp


$(WUHB_FILE): $(RPX_FILE)
	$(WUHBTOOL) $< $@ \
		--icon=assets/icon.png \
		--author="Daniel K. O." \
		--name="$(PACKAGE)" \
		--short-name="$(PACKAGE_TARNAME)-helper"


all-local: $(WUHB_FILE)


install-exec-local: $(WUHB_FILE)
	curl "ftp://wiiu:/fs/vol/external01/wiiu/apps/" --upload-file $<


uninstall-local:
	curl "ftp://wiiu" --quote "DELE /fs/vol/external01/wiiu/apps/$(WUHB_FILE)"


.PHONY: run
run: $(WUHB_FILE)
	WIILOAD=tcp:wiiu wiiload $<


.PHONY: company
company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > $(srcdir)/compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> $(srcdir)/compile_flags.txt
