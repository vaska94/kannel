dnl rabbitmqbox -- RabbitMQ message queue box for Kamex
dnl
dnl Copyright (c) 2026 Vasil Jamalashvili <shapeless@pm.me>

AC_PREREQ([2.69])
AC_INIT([rabbitmqbox], [1.0.0], [shapeless@pm.me])
AC_CONFIG_SRCDIR([gw/rabbitmqbox.c])
AC_CONFIG_HEADERS([rmq-config.h])
AC_CONFIG_AUX_DIR([autotools])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([m4])

dnl Checks for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
LT_INIT

dnl Check for Kamex installation
AC_ARG_WITH(kannel-dir,
[  --with-kannel-dir=DIR   where to look for Kamex/Kannel gateway libs and headers
                          DIR points to the installation [/usr/local] ],
[
    kanneldir="$withval"
],[
    kanneldir="/usr/local"
])

AC_SUBST(kanneldir)

dnl Check for Kamex headers
AC_MSG_CHECKING([for Kamex headers])
if test -f "$kanneldir/include/kamex/gw-config.h"; then
    AC_MSG_RESULT([found in $kanneldir/include/kamex])
    KANNEL_CFLAGS="-I$kanneldir/include/kamex"
elif test -f "$kanneldir/include/kannel/gw-config.h"; then
    AC_MSG_RESULT([found in $kanneldir/include/kannel])
    KANNEL_CFLAGS="-I$kanneldir/include/kannel"
else
    AC_MSG_ERROR([Kamex/Kannel headers not found. Use --with-kannel-dir=DIR])
fi

dnl Check for Kamex libraries
AC_MSG_CHECKING([for Kamex libraries])
if test -f "$kanneldir/lib/libgwlib.so" -o -f "$kanneldir/lib/libgwlib.a"; then
    AC_MSG_RESULT([found in $kanneldir/lib])
    KANNEL_LIBS="-L$kanneldir/lib -lgwlib -lgw"
else
    AC_MSG_ERROR([Kamex/Kannel libraries not found. Use --with-kannel-dir=DIR])
fi

AC_SUBST(KANNEL_CFLAGS)
AC_SUBST(KANNEL_LIBS)

dnl Check for librabbitmq
AC_MSG_CHECKING([for librabbitmq])
PKG_CHECK_MODULES([RABBITMQ], [librabbitmq >= 0.10.0], [
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_RABBITMQ], [1], [Define if you have librabbitmq])
], [
    dnl Try manual detection if pkg-config fails
    AC_CHECK_HEADER([amqp.h], [
        AC_CHECK_LIB([rabbitmq], [amqp_new_connection], [
            RABBITMQ_CFLAGS=""
            RABBITMQ_LIBS="-lrabbitmq"
            AC_DEFINE([HAVE_RABBITMQ], [1], [Define if you have librabbitmq])
            AC_MSG_RESULT([yes (manual)])
        ], [
            AC_MSG_ERROR([librabbitmq not found. Install librabbitmq-dev or rabbitmq-c-dev])
        ])
    ], [
        AC_MSG_ERROR([amqp.h not found. Install librabbitmq-dev or rabbitmq-c-dev])
    ])
])

AC_SUBST(RABBITMQ_CFLAGS)
AC_SUBST(RABBITMQ_LIBS)

dnl Check for pthread
AC_CHECK_LIB([pthread], [pthread_create], [
    PTHREAD_LIBS="-lpthread"
], [
    AC_MSG_ERROR([pthread library not found])
])
AC_SUBST(PTHREAD_LIBS)

dnl Check for OpenSSL (optional, for TLS)
AC_ARG_ENABLE([ssl],
    AS_HELP_STRING([--enable-ssl], [Enable SSL/TLS support for RabbitMQ]))

if test "x$enable_ssl" = "xyes"; then
    PKG_CHECK_MODULES([SSL], [openssl], [
        AC_DEFINE([HAVE_RABBITMQ_SSL], [1], [Define if you have OpenSSL for RabbitMQ])
    ], [
        AC_MSG_WARN([OpenSSL not found, SSL support disabled])
    ])
fi

dnl Output files
AC_CONFIG_FILES([
    Makefile
    gw/Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([
rabbitmqbox configuration:
  Kamex directory:    $kanneldir
  RabbitMQ CFLAGS:    $RABBITMQ_CFLAGS
  RabbitMQ LIBS:      $RABBITMQ_LIBS
  SSL support:        $enable_ssl
])
