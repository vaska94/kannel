AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I /usr/share/gettext/m4

SUBDIRS = gwlib gw utils test checks benchmarks

# TODO:
# install test (conditionally)
# test soap


EXTRA_DIST = ChangeLog INSTALL LICENSE README VERSION contrib 

BUILT_SOURCES = gw-config.h

includedir = $(prefix)/include/kannel

include_HEADERS = \
	gw-config.h

bin_SCRIPTS = gw-config

gw-config: Makefile
	@echo '#!/bin/sh' > $@
	@echo 'usage() { echo "usage: $$0 [--cflags] [--libs] [--version]"; exit 0; }' >> $@
	@echo 'test "$$1" || usage' >> $@
	@echo 'while test "$$1"; do' >> $@
	@echo '  case "$$1" in' >> $@
	@echo '    --cflags) echo "-I$(includedir) @CFLAGS@" ;;' >> $@
	@echo '    --libs) echo "-L$(libdir) -lgw -lgwlib $(LIBS)" ;;' >> $@
	@echo '    --version) echo "@VERSION@" ;;' >> $@
	@echo '    *) usage ;;' >> $@
	@echo '  esac; shift' >> $@
	@echo 'done' >> $@
	chmod 0755 $@


nag:
	utils/find-long-lines

pkg: all
	(cd solaris; mk-solaris-package.sh)
	echo Package is now in solaris directory

# Generate SSL certificates for testing (2048-bit RSA with SHA256)
ssl-certs:
	@echo "Generating SSL certificates for testing..."
	@cd gw && openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem \
		-days 365 -nodes -subj "/C=US/ST=Test/L=Test/O=Kannel/OU=Test/CN=localhost" \
		2>/dev/null
	@echo "SSL certificates generated: gw/cert.pem and gw/key.pem"
	@echo "Note: These are self-signed certificates for testing only"

# Clean SSL certificates
ssl-certs-clean:
	@echo "Removing SSL certificates..."
	@rm -f gw/cert.pem gw/key.pem
	@echo "SSL certificates removed"

dist-hook:
	rm -rf `find $(distdir) -name CVS -or -name SCCS or -name .svn`

