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
	smsc/emimsg.c \
	smsc/smpp_pdu.c

sbin_PROGRAMS = bearerbox smsbox

bearerbox_LDADD = $(top_builddir)/gwlib/libgwlib.la libgw.la
bearerbox_SOURCES = \
	bb_alog.c \
	bb_boxc.c \
	bb_http.c \
	bb_smscconn.c \
	smscconn.c \
	smsc/smsc_at.c \
	smsc/smsc_emi.c \
	smsc/smsc_fake.c \
	smsc/http/generic.c \
	smsc/http/kannel.c \
	smsc/smsc_http.c \
	smsc/smsc_loopback.c \
	smsc/smsc_smpp.c \
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
	
includedir = $(prefix)/include/kamex/gw

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
	smsc/emimsg.h \
	smsc/smpp_pdu.h \
	smsc/smsc_at.h \
	smsc/smsc_http_p.h \
	smsc/smpp_pdu.def \
	smsc/smpp_pdu_opt.def

	
