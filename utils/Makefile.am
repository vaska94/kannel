AM_CFLAGS = -I. -I$(top_builddir)/gw -I$(top_builddir)/gwlib -I$(top_builddir)

bin_PROGRAMS = mtbatch decode_emimsg
sbin_PROGRAMS = run_kannel_box

man1_MANS = mtbatch.1
man8_MANS = run_kannel_box.8


mtbatch_LDADD = $(top_builddir)/gwlib/libgwlib.la $(top_builddir)/gw/libgw.la
mtbatch_SOURCES = \
		mtbatch.c

decode_emimsg_LDADD = $(top_builddir)/gwlib/libgwlib.la $(top_builddir)/gw/libgw.la
decode_emimsg_SOURCES = \
		decode_emimsg.c

run_kannel_box_LDADD = $(top_builddir)/gwlib/libgwlib.la $(top_builddir)/gw/libgw.la
run_kannel_box_SOURCES = \
		run_kannel_box.c

if BUILD_STARTSTOPDAEMON

sbin_PROGRAMS += start-stop-daemon
start_stop_daemon_LDADD = $(top_builddir)/gwlib/libgwlib.la $(top_builddir)/gw/libgw.la
start_stop_daemon_SOURCES = \
		start-stop-daemon.c

endif