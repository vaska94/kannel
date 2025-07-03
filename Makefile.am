AUTOMAKE_OPTIONS = foreign

SUBDIRS = gwlib gw utils test doc checks benchmarks

# TODO:
# install test (conditionally)
# test soap
# test doc


EXTRA_DIST = AUTHORS ChangeLog COPYING INSTALL LICENSE NEWS README STATUS VERSION contrib 

BUILT_SOURCES = gw-config.h

includedir = $(prefix)/include/kannel

include_HEADERS = \
	gw-config.h

bin_SCRIPTS = gw-config

gw-config: utils/foobar-config.sh
	./utils/foobar-config.sh "-I$(includedir) @CFLAGS@" \
		"-L$(libdir) -lgw -lgwlib $(LIBS)" \
		"@VERSION@" > gw-config
	chmod 0755 gw-config


nag:
	utils/find-long-lines

pkg: all
	(cd solaris; mk-solaris-package.sh)
	echo Package is now in solaris directory

dist-hook:
	rm -rf `find $(distdir) -name CVS -or -name SCCS or -name .svn`

