dnl SQLBox - Database queue box for Kamex SMS Gateway
dnl
dnl Originally by Rene Kluwen, Martin Conte, Alejandro Guerrieri, Stipe Tolj
dnl Modernized for Kamex project

AC_PREREQ([2.69])
AC_INIT([kamex-sqlbox], [1.7.7], [dev@kamex.dev])
AC_CONFIG_SRCDIR([gw/sqlbox.c])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_HEADERS([sb-config.h])

AM_INIT_AUTOMAKE([foreign -Wall])
AM_MAINTAINER_MODE([enable])
AC_CANONICAL_HOST

dnl Checks for programs
AC_PROG_CC
AM_PROG_AR
LT_INIT

dnl Add include paths
AM_CPPFLAGS='-I$(top_srcdir)/gw -I$(top_builddir)/gw'
AC_SUBST([AM_CPPFLAGS])

dnl Checks for header files
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_TYPE_SIZE_T

dnl Platform-specific flags
EXE_EXT=""
case "$host" in
  *-cygwin*)
    EXE_EXT=".exe"
    LDFLAGS="$LDFLAGS -Wl,--enable-auto-import"
    ;;
  *apple-darwin*)
    CFLAGS="$CFLAGS -DDARWIN=1"
    ;;
  *-linux-*)
    CFLAGS="$CFLAGS -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE"
    LDFLAGS="$LDFLAGS -rdynamic"
    ;;
  *-*-openbsd* | *-*-freebsd*)
    CFLAGS="$CFLAGS -pthread"
    ;;
esac

AC_ARG_WITH(cflags,
[  --with-cflags=FLAGS     use FLAGS for CFLAGS],
CFLAGS="$CFLAGS $withval")

AC_ARG_WITH(libs,
[  --with-libs=FLAGS       use FLAGS for extra libraries],
LIBS="$LIBS $withval")


dnl ===========================================================================
dnl OpenSSL support
dnl ===========================================================================

AC_ARG_WITH(ssl,
[  --with-ssl[=DIR]        where to look for OpenSSL libs and header files
                          DIR points to the installation [/usr/local/ssl]],
[ if test -d "$withval"; then
    ssllib="$withval/lib";
    sslinc="$withval/include"
  else
    AC_MSG_ERROR(Unable to find OpenSSL libs and/or directories at $withval)
  fi
])

AC_MSG_CHECKING([whether to compile with SSL support])
AC_ARG_ENABLE(ssl,
[  --enable-ssl            enable SSL client and server support [enabled]], [
  if test "$enableval" = no ; then
    AC_MSG_RESULT(disabled)
    ssl=no
  else
    ssl=yes
  fi
],[
  ssl=yes
])

if test "$ssl" = "yes" ; then
  if test "x$ssllib" = "x" && test "x$sslinc" = "x"; then
    PKG_CHECK_MODULES([SSL], [openssl], [
      CFLAGS="$CFLAGS $SSL_CFLAGS"
      LIBS="$LIBS $SSL_LIBS"
      AC_DEFINE(HAVE_LIBSSL, 1, [Define if OpenSSL is available])
      AC_MSG_RESULT(yes - via pkg-config)
    ], [
      for loc in /usr/lib /usr/lib64 /usr/local/ssl/lib /usr/local/openssl/lib; do
        if test -f "$loc/libssl.so" -o -f "$loc/libssl.a"; then
          ssllib="$loc"
          break
        fi
      done
      for loc in /usr/include /usr/include/openssl /usr/local/ssl/include /usr/local/openssl/include; do
        if test -d "$loc" && test -f "$loc/openssl/ssl.h" -o -f "$loc/ssl.h"; then
          sslinc="$loc"
          break
        fi
      done
    ])
  fi

  if test "x$ssllib" != "x" && test "x$sslinc" != "x"; then
    CFLAGS="$CFLAGS -I$sslinc"
    LIBS="$LIBS -L$ssllib -lssl -lcrypto"
    AC_DEFINE(HAVE_LIBSSL, 1, [Define if OpenSSL is available])
    AC_MSG_RESULT(yes)
  fi
fi


dnl ===========================================================================
dnl Sybase CT-Lib support
dnl ===========================================================================

AC_MSG_CHECKING(for Ct-Lib support)
AC_ARG_WITH(ctlib,
[  --with-ctlib[=DIR]      Include Ct-Lib support.  DIR is the Ct-Lib
                          install directory, defaults to /opt/sybase.],
[
    if test "$withval" = "yes"; then
        withval=/opt/sybase
    fi

    if test "$withval" != "no"; then
        if test -f $withval/include/ctpublic.h; then
            CTLIB_INCDIR=$withval/include
            CTLIB_LIBDIR=$withval/lib
        else
            AC_MSG_RESULT(no)
            AC_MSG_ERROR(Invalid Ct-Lib directory - unable to find ctpublic.h)
        fi
        CTLIB_LFLAGS="-L$CTLIB_LIBDIR -lct -lcs -lsybtcl -lcomn -lintl"
        CTLIB_INCLUDE="-I$CTLIB_INCDIR"
        AC_DEFINE(HAVE_CTLIB, 1, [Define if CT-Lib is available])
        AC_MSG_RESULT(yes)
        have_db=yes
        AC_CHECK_FUNC(dlopen, , AC_CHECK_LIB(dl, dlopen, CTLIB_LFLAGS="$CTLIB_LFLAGS -ldl"))
    fi
],[
    AC_MSG_RESULT(no)
])
AC_SUBST(CTLIB_LFLAGS)
AC_SUBST(CTLIB_INCLUDE)


dnl ===========================================================================
dnl FreeTDS CT-Lib support (for MSSQL)
dnl ===========================================================================

AC_MSG_CHECKING(for FreeTDS Ct-Lib support)
AC_ARG_WITH(mssql,
[  --with-mssql[=DIR]      Include FreeTDS Ct-Lib support.  DIR is the FreeTDS
                          install directory, defaults to /usr/local.],
[
    if test "$withval" = "yes"; then
        withval=/usr/local
    fi

    if test "$withval" != "no"; then
        if test -f $withval/include/ctpublic.h; then
            CTLIB_INCDIR=$withval/include
            CTLIB_LIBDIR=$withval/lib
        else
            AC_MSG_RESULT(no)
            AC_MSG_ERROR(Invalid FreeTDS directory - unable to find ctpublic.h)
        fi
        CTLIB_LFLAGS="-L$CTLIB_LIBDIR -lct"
        CTLIB_INCLUDE="-I$CTLIB_INCDIR"
        AC_DEFINE(HAVE_CTLIB, 1, [Define if CT-Lib is available])
        AC_MSG_RESULT(yes)
        have_db=yes
        AC_CHECK_FUNC(dlopen, , AC_CHECK_LIB(dl, dlopen, CTLIB_LFLAGS="$CTLIB_LFLAGS -ldl"))
    fi
],[
    AC_MSG_RESULT(no)
])
AC_SUBST(CTLIB_LFLAGS)
AC_SUBST(CTLIB_INCLUDE)


dnl ===========================================================================
dnl Kamex Gateway detection
dnl ===========================================================================

AC_ARG_WITH(kannel-dir,
[  --with-kannel-dir=DIR   where to look for Kamex libs and headers [/usr/local]],
[
  if test -d "$withval" ; then
    gwloc="$withval"
  fi
])

AC_PATH_PROG(GW_CONFIG, gw-config, no, [$gwloc/bin:$gwloc:$PATH])

if test "$GW_CONFIG" = "no"; then
  dnl Try to find headers directly
  found=""
  for loc in $gwloc /usr /usr/local ; do
    if test "x$found" = "x" ; then
      AC_MSG_CHECKING([for Kamex include files in $loc])
      if test -f "$loc/include/kamex/gw-config.h" ; then
        CFLAGS="$CFLAGS -I$loc/include/kamex"
        LDFLAGS="$LDFLAGS -L$loc/lib"
        LIBS="$LIBS -lgwlib -lgw"
        found=1
        AC_MSG_RESULT([found])
      else
        AC_MSG_RESULT([no])
      fi
    fi
  done
  if test "x$found" != "x1" ; then
    AC_MSG_ERROR([Unable to find gw-config.h, please provide --with-kannel-dir=<dir>])
  fi
else
  dnl Use gw-config
  AC_MSG_CHECKING([Kamex version])
  gw_version=`$GW_CONFIG --version`
  AC_MSG_RESULT([$gw_version])

  gw_libs=`$GW_CONFIG --libs 2>/dev/null`
  if test "x$gw_libs" != "x" ; then
    LIBS="$LIBS $gw_libs"
  fi

  gw_cflags=`$GW_CONFIG --cflags 2>/dev/null`
  if test "x$gw_cflags" != "x" ; then
    CFLAGS="$CFLAGS $gw_cflags"
  fi
fi

AC_CHECK_LIB([gwlib], [cfg_create], [],
  [AC_MSG_ERROR([Kamex gwlib is required!])], [$LDFLAGS])


dnl ===========================================================================
dnl Check for database support in Kamex
dnl ===========================================================================

AC_MSG_CHECKING([for Kamex database support])

dnl These are provided by Kamex's gwlib - we just need to link against it
dnl SQLBox uses: MySQL, PostgreSQL, SQLite3, Oracle, MSSQL
AC_MSG_RESULT([using Kamex database backends])

dnl Check which databases are available in the installed Kamex
AC_CHECK_DECL([HAVE_MYSQL], [have_mysql=yes], [have_mysql=no], [#include "gw-config.h"])
AC_CHECK_DECL([HAVE_PGSQL], [have_pgsql=yes], [have_pgsql=no], [#include "gw-config.h"])
AC_CHECK_DECL([HAVE_SQLITE3], [have_sqlite3=yes], [have_sqlite3=no], [#include "gw-config.h"])
AC_CHECK_DECL([HAVE_ORACLE], [have_oracle=yes], [have_oracle=no], [#include "gw-config.h"])

AC_MSG_NOTICE([Database support from Kamex:])
AC_MSG_NOTICE([  MySQL:    $have_mysql])
AC_MSG_NOTICE([  PgSQL:    $have_pgsql])
AC_MSG_NOTICE([  SQLite3:  $have_sqlite3])
AC_MSG_NOTICE([  Oracle:   $have_oracle])


dnl ===========================================================================
dnl Final Output
dnl ===========================================================================

AC_CONFIG_FILES([Makefile gw/Makefile])
AC_OUTPUT

cat <<EOF

+--------------------------------------------------------------------+
| SQLBox for Kamex SMS Gateway                                       |
| https://kamex.dev                                                  |
+--------------------------------------------------------------------+

Configuration complete. Run 'make' to build.
EOF
