@INC_AMINCLUDE@
DISTCLEANFILES = $(AMINCLUDE)


SUBDIRS = \
	external/libwupsxx \
	helper-app


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


AM_CPPFLAGS = \
	$(DEVKITPRO_CPPFLAGS) \
	-Iexternal/libwupsxx/include

AM_CXXFLAGS = \
	$(DEVKITPRO_CXXFLAGS) \
	-Wall -Wextra

AM_LDFLAGS = $(DEVKITPRO_LDFLAGS)

LIBS = $(DEVKITPRO_LIBS)


WPS_FILE = system-font-replacer.wps
CLEANFILES = $(WPS_FILE)

noinst_PROGRAMS = system-font-replacer.elf

system_font_replacer_elf_SOURCES = src/main.cpp

system_font_replacer_elf_LDADD = external/libwupsxx/src/libwupsxx.a


all-local: $(WPS_FILE)


install-exec-local: $(WPS_FILE)
	curl "ftp://wiiu:/fs/vol/external01/wiiu/environments/aroma/plugins/" --upload-file $<


uninstall-local:
	curl "ftp://wiiu" --quote "DELE /fs/vol/external01/wiiu/environments/aroma/plugins/$(WPS_FILE)"


.PHONY: run
run: all
	WIILOAD=tcp:wiiu wiiload $(WPS_FILE)


.PHONY: company
company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > $(srcdir)/compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> $(srcdir)/compile_flags.txt

