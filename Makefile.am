AUTOMAKE_OPTIONS = foreign

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

