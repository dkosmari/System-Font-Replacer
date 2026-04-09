EXTRA_DIST = \
	assets \
	AUTHORS \
	bootstrap \
	copy-pua.py \
	COPYING \
	docker-build.sh \
	Dockerfile \
	merge-fonts.py \
	README.md


SUBDIRS = \
	external/libwupsxx \
	helper-app


AM_CPPFLAGS = \
	$(WUPS_CPPFLAGS) \
	-I$(srcdir)/external/libwupsxx/include

AM_CXXFLAGS = -Wall -Wextra -Werror

AM_LDFLAGS = $(WUPS_LDFLAGS)

LDADD = \
	external/libwupsxx/lib/libwupsxx.la \
	$(WUPS_LIB)


ELF_FILE := system-font-replacer.elf
WPS_FILE := $(ELF_FILE:.elf=.wps)


noinst_PROGRAMS = $(ELF_FILE)

system_font_replacer_elf_SOURCES = src/main.cpp


all-local: $(WPS_FILE)


.PHONY: run wiiu-install wiiu-uninstall company


run: $(WPS_FILE)
	WIILOAD=tcp:wiiu wiiload $<


wiiu-install: $(WPS_FILE)
	curl "ftp://wiiu:/fs/vol/external01/wiiu/environments/aroma/plugins/" --upload-file $<


wiiu-uninstall:
	curl "ftp://wiiu" --quote "DELE /fs/vol/external01/wiiu/environments/aroma/plugins/$(WPS_FILE)"


company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> compile_flags.txt


@INC_AMINCLUDE@

CLEANFILES = $(WPS_FILE)
DISTCLEANFILES = $(AMINCLUDE)
