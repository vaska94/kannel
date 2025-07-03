AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CFLAGS = -I. -I$(top_builddir)/gwlib -I$(top_builddir)

lib_LTLIBRARIES = libgw.la

libgw_la_LIBADD = $(top_builddir)/gwlib/libgwlib.la
libgw_la_SOURCES = \
	bb_store.c \
	bb_store_file.c \
	bb_store_redis.c \
	bb_store_spool.c \
	dlr.c \
	dlr_cass.c \
	dlr_mem.c \
	dlr_mssql.c \
	dlr_mysql.c \
	dlr_oracle.c \
	dlr_pgsql.c \
	dlr_redis.c \
	dlr_sdb.c \
	dlr_spool.c \
	dlr_sqlite3.c \
	heartbeat.c \
	html.c \
	load.c \
	meta_data.c \
	msg.c \
	numhash.c \
	shared.c \
	sms.c \
	urltrans.c \
	xml_shared.c \
	smsc/emimsg.c \
	smsc/smasi_pdu.c \
	smsc/smpp_pdu.c

sbin_PROGRAMS = bearerbox smsbox
man8_MANS = kannel.8

EXTRA_DIST = $(man8_MANS)

bearerbox_LDADD = $(top_builddir)/gwlib/libgwlib.la libgw.la
bearerbox_SOURCES = \
	bb_alog.c \
	bb_boxc.c \
	bb_http.c \
	bb_smscconn.c \
	bb_udp.c \
	smscconn.c \
	smsc/smsc.c \
	smsc/smsc_at.c \
	smsc/smsc_cgw.c \
	smsc/smsc_cimd.c \
	smsc/smsc_cimd2.c \
	smsc/smsc_emi.c \
	smsc/smsc_emi_x25.c \
	smsc/smsc_fake.c \
	smsc/http/brunet.c \
	smsc/http/clickatell.c \
	smsc/http/generic.c \
	smsc/http/kannel.c \
	smsc/http/xidris.c \
	smsc/smsc_http.c \
	smsc/smsc_loopback.c \
	smsc/smsc_ois.c \
	smsc/smsc_oisd.c \
	smsc/smsc_sema.c \
	smsc/smsc_smasi.c \
	smsc/smsc_smpp.c \
	smsc/smsc_wrapper.c \
	bearerbox.c \
	bearerbox.h

#if BUILD_SOAP not ready TODO
## if defined GSOAP add it
#bearerbox_SOURCES += smsc/smsc_soap_parlayx.c $(soap_depend)
#bearerbox_LDADD   += $(top_builddir)/soap/libsoap_plugin.a
## end
#endif

smsbox_LDADD = $(top_builddir)/gwlib/libgwlib.la libgw.la
smsbox_SOURCES = \
	smsbox.c
	
includedir = $(prefix)/include/kannel/gw

nobase_include_HEADERS = \
	alt_charsets.h \
	bb.h \
	bb_smscconn_cb.h \
	bb_store.h \
	dlr.h \
	dlr_p.h \
	heartbeat.h \
	html.h \
	load.h \
	meta_data.h \
	msg-decl.h \
	msg.h \
	numhash.h \
	shared.h \
	sms.h \
	smscconn.h \
	smscconn_p.h \
	urltrans.h \
	xml_definitions.h \
	xml_shared.h \
	smsc/emimsg.h \
	smsc/smasi_pdu.h \
	smsc/smpp_pdu.h \
	smsc/smsc.h \
	smsc/smsc_at.h \
	smsc/smsc_p.h \
	smsc/smsc_http_p.h \
	smsc/smsc_sema.h \
	smsc/smasi_pdu.def \
	smsc/smpp_pdu.def \
	smsc/smpp_pdu_opt.def

	
