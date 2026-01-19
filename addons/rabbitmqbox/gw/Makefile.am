sbin_PROGRAMS = rabbitmqbox

rabbitmqbox_SOURCES = \
	rabbitmqbox.c \
	rmq_connection.c \
	rmq_connection.h

rabbitmqbox_CFLAGS = \
	$(KANNEL_CFLAGS) \
	$(RABBITMQ_CFLAGS) \
	-I$(top_srcdir)

rabbitmqbox_LDADD = \
	$(KANNEL_LIBS) \
	$(RABBITMQ_LIBS) \
	$(PTHREAD_LIBS)
