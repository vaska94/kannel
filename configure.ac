dnl /* ==================================================================== 
dnl  * The Kannel Software License, Version 1.0 
dnl  * 
dnl  * Copyright (c) 2001-2018 Kannel Group  
dnl  * Copyright (c) 1998-2001 WapIT Ltd.   
dnl  * All rights reserved. 
dnl  * 
dnl  * Redistribution and use in source and binary forms, with or without 
dnl  * modification, are permitted provided that the following conditions 
dnl  * are met: 
dnl  * 
dnl  * 1. Redistributions of source code must retain the above copyright 
dnl  *    notice, this list of conditions and the following disclaimer. 
dnl  * 
dnl  * 2. Redistributions in binary form must reproduce the above copyright 
dnl  *    notice, this list of conditions and the following disclaimer in 
dnl  *    the documentation and/or other materials provided with the 
dnl  *    distribution. 
dnl  * 
dnl  * 3. The end-user documentation included with the redistribution, 
dnl  *    if any, must include the following acknowledgment: 
dnl  *       "This product includes software developed by the 
dnl  *        Kannel Group (http://www.kannel.org/)." 
dnl  *    Alternately, this acknowledgment may appear in the software itself, 
dnl  *    if and wherever such third-party acknowledgments normally appear. 
dnl  * 
dnl  * 4. The names "Kannel" and "Kannel Group" must not be used to 
dnl  *    endorse or promote products derived from this software without 
dnl  *    prior written permission. For written permission, please  
dnl  *    contact org@kannel.org. 
dnl  * 
dnl  * 5. Products derived from this software may not be called "Kannel", 
dnl  *    nor may "Kannel" appear in their name, without prior written 
dnl  *    permission of the Kannel Group. 
dnl  * 
dnl  * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED 
dnl  * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
dnl  * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
dnl  * DISCLAIMED.  IN NO EVENT SHALL THE KANNEL GROUP OR ITS CONTRIBUTORS 
dnl  * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,  
dnl  * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  
dnl  * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR  
dnl  * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,  
dnl  * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE  
dnl  * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  
dnl  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
dnl  * ==================================================================== 
dnl  * 
dnl  * This software consists of voluntary contributions made by many 
dnl  * individuals on behalf of the Kannel Group.  For more information on  
dnl  * the Kannel Group, please see <http://www.kannel.org/>. 
dnl  * 
dnl  * Portions of this software are based upon software originally written at  
dnl  * WapIT Ltd., Helsinki, Finland for the Kannel project.  
dnl  */ 

dnl
dnl configure.ac -- main autoconf macro definition file
dnl
dnl Process this file with autoconf to produce a configure script.
dnl

dnl initialization

AC_PREREQ([2.69])

dnl Check gateway version number early for AC_INIT
m4_define([KANNEL_VERSION], m4_esyscmd_s([head -n 1 VERSION]))

AC_INIT([Kannel], [KANNEL_VERSION], [shapeless@pm.me])
AC_CONFIG_SRCDIR([gw/alt_charsets.h])
AC_CONFIG_HEADERS([gw-config.h])
AC_SUBST(SHELL)
AC_CONFIG_AUX_DIR([.])
AC_SET_TERMINAL_SEQUENCES()
AC_CONFIG_NICE([config.nice])

dnl Check gateway version number.
VERSION="KANNEL_VERSION"
if test "x$VERSION" = "xdev" -o "x$VERSION" = "xgit"; then
  AC_MSG_CHECKING([git revision])
  AC_GIT_REVISION(GIT_REVISION)
  AC_MSG_RESULT([$GIT_REVISION])
  VERSION="$VERSION-$GIT_REVISION"
fi
AC_DEFINE_UNQUOTED([GW_NAME], ["Kannel"], [Gateway name])
AC_DEFINE_UNQUOTED([GW_VERSION], ["$VERSION"], [Gateway version])
AC_SUBST([GW_VERSION], [$VERSION])

AC_CONFIG_SECTION([Configuring for Kannel gateway version $VERSION])

AM_INIT_AUTOMAKE([foreign -Wall])
AM_PROG_AR
AC_LANG([C])
AC_DISABLE_STATIC
LT_INIT
dnl AC_SUBST(LIBTOOL_DEPS)

dnl Target installation directory for documentation
AC_SUBST(docdir)
docdir='${prefix}/share/doc/kannel'

dnl Checks system type.
AC_CONFIG_SECTION([Running system checks])
AC_CANONICAL_HOST

dnl Checks for programs.

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_YACC
AC_PROG_LEX([noyywrap])
AC_PATH_PROG(CONVERT, convert)
AC_PATH_PROG(PERL, perl)

# Check whether `pkg-config' is available
AC_ARG_VAR([PKG_CONFIG], [path to pkg-config])
AC_ARG_VAR([PKG_CONFIG_PATH], [directories to add to the pkg-config search path])
AC_ARG_VAR([PKG_CONFIG_LIBDIR], [path overriding pkg-config's search path])

if test "x$ac_cv_env_PKG_CONFIG_set" != "xset"; then
    AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
fi

dnl Apply system specific rules.
dnl Executable extension for systems that need one, i.e. Cygwin
EXE_EXT=""
case "$host" in
  *-cygwin*)
    EXE_EXT=".exe"
    LDFLAGS="$LDFLAGS -Wl,--enable-auto-import"
    ;;
  *apple-darwin*)
    CFLAGS="$CFLAGS -DDARWIN=1"
    #LIBTOOL="libtool -static -o"
    ;;
  *-linux-*)
    CFLAGS="$CFLAGS -D_XOPEN_SOURCE=600 -D_BSD_SOURCE -D_DEFAULT_SOURCE"
    LDFLAGS="$LDFLAGS -rdynamic"
    ;;
  *-*-openbsd* | *-*-freebsd*)
    CFLAGS="$CFLAGS -pthread"
    AC_CHECK_LIB(pthread, pthread_exit, [LIBS="$LIBS -lpthread"; pthread="yes"])
    ;;
esac
AC_SUBST(EXE_EXT)


AC_ARG_WITH(cflags,
[  --with-cflags=FLAGS     use FLAGS for CFLAGS],
CFLAGS="$CFLAGS $withval")

AC_ARG_WITH(libs,
[  --with-libs=FLAGS       use FLAGS for extra libraries],
LIBS="$LIBS $withval")

dnl Check whether compiler supports inline
AC_C_INLINE

dnl Check for how to do large files

AC_SYS_LARGEFILE

dnl Check for word sizes.

AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)
SIZEOF_SHORT=$ac_cv_sizeof_short
SIZEOF_INT=$ac_cv_sizeof_int
SIZEOF_LONG=$ac_cv_sizeof_long
SIZEOF_LONG_LONG=$ac_cv_sizeof_long_long
AC_SUBST(SIZEOF_SHORT)
AC_SUBST(SIZEOF_INT)
AC_SUBST(SIZEOF_LONG)
AC_SUBST(SIZEOF_LONG_LONG)

dnl Checks for libraries.

AC_CHECK_LIB(m, log)
AC_CHECK_LIB(socket, accept)
AC_CHECK_LIB(nsl, inet_ntoa)
AC_CHECK_LIB(resolv, inet_ntop)
AC_CHECK_LIB(bind, inet_ntop)
if test -z "$pthread"; then
    AC_CHECK_LIB(pthread, pthread_exit, [LIBS="$LIBS -lpthread"])
fi
dnl Check if iconv is in libc first, only add -liconv if needed
AC_CHECK_FUNC(iconv_open, [],
  [AC_CHECK_LIB(iconv, iconv_open, [LIBS="$LIBS -liconv"])])

dnl Checks for header files.

AC_CHECK_HEADERS(sys/ioctl.h sys/time.h sys/types.h unistd.h sys/poll.h)
AC_CHECK_HEADERS(pthread.h getopt.h syslog.h iconv.h zlib.h execinfo.h stdlib.h)
AC_CHECK_HEADERS([sys/socket.h sys/sockio.h netinet/in.h])
AC_CHECK_HEADERS([net/if.h], [], [],
  [#include <stdio.h>
   #if STDC_HEADERS
   #  include <stdlib.h>
   #  include <stddef.h>
   #else
   #if HAVE_STDLIB_H
   #  include <stdlib.h>
   # endif
   #endif
   #if HAVE_SYS_SOCKET_H
   # include <sys/socket.h>
   #endif
  ])
AC_CHECK_HEADERS([sys/ucontext.h], [],
  [AC_MSG_WARN(System lacks ucontext.h - user context support)]
)

dnl Checks for typedefs, structures, and compiler characteristics.

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [char *func = __FUNCTION__;])],
    [AC_DEFINE([HAVE___FUNCTION__], [1], [Do we have __FUNCTION__])])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [char *func = __func__;])],
    [AC_DEFINE([HAVE___FUNC__], [1], [Do we have __func__])])

dnl check for iconv
AM_ICONV

dnl Checks for library functions.

AC_CHECK_FUNCS(gettimeofday select socket strdup getopt_long localtime_r gmtime_r backtrace srandom strtoll strtoq setpriority initgroups)
AC_CHECK_FUNC(dlopen, [], AC_CHECK_LIB(dl, dlopen, LDFLAGS="$LDFLAGS -ldl"))

dnl Check if we have reentrant gethostbyname and which one
AC_CHECK_FUNC(gethostbyname_r, [ AC_FUNC_WHICH_GETHOSTBYNAME_R ], [
  AC_CHECK_FUNC(gethostbyname,[], [
    AC_MSG_ERROR([Couldnot find gethostbyname_r nor gethostbyname functions])])]
)

dnl Extra feature checks

GW_HAVE_TYPE_FROM(sys/socket.h,
	socklen_t,
	HAVE_SOCKLEN_T,
	[Defined if there is a socklen_t in <sys/socket.h>])

GW_HAVE_FUNC_FROM(stdio.h, getopt, HAVE_GETOPT_IN_STDIO_H,
    	    	  [Does <stdio.h> declare getopt()?])
GW_HAVE_FUNC_FROM(unistd.h, getopt, HAVE_GETOPT_IN_UNISTD_H,
    	    	  [Does <unistd.h> declare getopt()?])

dnl POSIX nftw()

AC_CHECK_HEADERS(ftw.h, [
  AC_CHECK_FUNC(nftw, [ 
    AC_DEFINE(HAVE_NFTW, 1, [Do we have NFTW])
  ])
])


dnl POSIX regular expression check

AC_CHECK_HEADERS(regex.h, [
  AC_CHECK_FUNC(regcomp, [ 
    AC_DEFINE(HAVE_REGEX, 1, [Do we have REGEX])
    has_posix_regex=1
  ])
])


dnl Misc feature checks

AC_CONFIG_SECTION([Checking for POSIX threads support])

AC_MSG_CHECKING(for working pthreads)
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
int pid;
void testpid(void* foo);
int main(void){
    pthread_t child;
    pid=getpid();
    pthread_create(&child,NULL,(void*)testpid,NULL);
    pthread_join(child,NULL);
    return 0;
}
void testpid(void* foo){
    int mypid=getpid();
    if(mypid!=pid){
        exit(1);
    }else{
        exit(0);
    }
}
]])],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no); CFLAGS="$CFLAGS -DBROKEN_PTHREADS=1"],
  [AC_MSG_RESULT([cross compiling - assuming they work])])

dnl
dnl Checking pthread_spin support
dnl
AC_MSG_CHECKING([for pthread_spinlock support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <pthread.h>]], [[
pthread_spinlock_t lock;
pthread_spin_init(&lock, 0);
pthread_spin_lock(&lock);
pthread_spin_unlock(&lock);
pthread_spin_destroy(&lock);
]])],
  [AC_MSG_RESULT(yes); AC_DEFINE([HAVE_PTHREAD_SPINLOCK_T], [1], [Do we have pthread_spinlock])],
  [AC_MSG_RESULT(no)])

dnl checking pthread_rwlock support
AC_MSG_CHECKING([for pthread_rwlock support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <pthread.h>]], [[
pthread_rwlock_t lock;
pthread_rwlock_init(&lock, NULL);
pthread_rwlock_rdlock(&lock);
pthread_rwlock_unlock(&lock);
pthread_rwlock_wrlock(&lock);
pthread_rwlock_unlock(&lock);
pthread_rwlock_destroy(&lock);
]])],
  [AC_MSG_RESULT(yes); AC_DEFINE([HAVE_PTHREAD_RWLOCK], [1], [Do we have pthread_rwlock])],
  [AC_MSG_RESULT(no)])

dnl checking for native semaphore support
AC_CHECK_LIB(rt, sem_init)
AC_MSG_CHECKING([for semaphore support])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <semaphore.h>
int main(void) {
    sem_t s;
    int val;
    if (sem_init(&s, 0, 1) != 0)
        return 1;
    sem_wait(&s);
    sem_post(&s);
    sem_getvalue(&s, &val);
    sem_destroy(&s);
    return 0;
}
]])],
  [AC_MSG_RESULT(yes); AC_DEFINE([HAVE_SEMAPHORE], [1], [Do we have semaphores])],
  [AC_MSG_RESULT(no)],
  [AC_MSG_RESULT([cross compiling - assuming supported]); AC_DEFINE([HAVE_SEMAPHORE], [1])])

dnl Implement the --enable-pcre option. This will set HAVE_PCRE in gw-config.h
dnl accordingly and enable the usage of Perl compatible regular expressions.
dnl As POSIX regex is a subset of PCRE, we map the posixpcre API calls, so 
dnl HAVE_REGEX has to be undefined. 

AC_CONFIG_SECTION([Configuring for PCRE support])
AC_MSG_CHECKING([whether to compile with PCRE support])
AC_ARG_ENABLE(pcre,
[  --enable-pcre           enable PCRE regex support @<:@disabled@:>@], [
  if test "$enableval" != yes; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(searching)
    AC_PATH_PROGS(PCRE_CONFIG, pcre-config, no)
    if test "$PCRE_CONFIG" = "no"; then
      AC_MSG_ERROR(Unable to find pcre-config in path for PCRE support)
    else
      AC_MSG_CHECKING([PCRE version])
      pcre_version=`$PCRE_CONFIG --version`
      AC_MSG_RESULT([$pcre_version])
      LIBS="$LIBS `$PCRE_CONFIG --libs-posix`"
      CFLAGS="$CFLAGS `$PCRE_CONFIG --cflags-posix`"
      AC_MSG_CHECKING([for POSIX regex provider])
      AC_MSG_RESULT([PCRE library])
      AC_CHECK_HEADERS(pcreposix.h)
      AC_CHECK_LIB(pcreposix, regcomp)
      AC_CHECK_HEADERS(pcre.h)
      AC_CHECK_FUNCS(pcre_compile)
      AC_DEFINE(HAVE_PCRE, 1,[Do we have PCRE])
      AC_DEFINE_UNQUOTED(LIBPCRE_VERSION, "$pcre_version", [Pcre version])
      PCRE="yes"
    fi
  fi
],[
  AC_MSG_RESULT(disabled)
])
                                       

dnl Implement --enable-warnings option.

AC_ARG_ENABLE(warnings,
[  --enable-warnings       enable compilation warnings @<:@disabled@:>@], [
  echo enabling compilation warnings
  if test -n "$GCC"; then
    CFLAGS="$CFLAGS -Wall"
    CFLAGS="$CFLAGS -Wmissing-prototypes"
    CFLAGS="$CFLAGS -Wmissing-declarations"
    CFLAGS="$CFLAGS -Wnested-externs"
    CFLAGS="$CFLAGS -Winline"
    CFLAGS="$CFLAGS -Wformat -Wformat-security -Wmissing-format-attribute"
    #CFLAGS="$CFLAGS -Wstrict-prototypes"
    #CFLAGS="$CFLAGS -Wredundant-decls"
    #CFLAGS="$CFLAGS -Wconversion"
  fi
])


AC_CONFIG_SECTION([Configuring parameters])

dnl Implement --with-defaults=speed/debug option.

AC_ARG_WITH(defaults,
[  --with-defaults=OPTION  set default configure options: speed/debug @<:@speed@:>@
                          this will set assertion checking and malloc wrapper accordingly
			    speed = native malloc + no assertions
			    debug = checking malloc + assertions], [
  case "$withval" in
  speed) assertiondefault=no
         mallocdefault=native
         ;;
  debug) assertiondefault=yes
         mallocdefault=check
         ;;
  *) echo "unknown --with-defaults parameter $withval"
     exit 1
     ;;
  esac
], [
  dnl defaults to native malloc but with assertions
  assertiondefault=yes
  mallocdefault=native
])


dnl Implement --with-malloc=[native|check|slow] option.

AC_MSG_CHECKING(which malloc to use)
AC_ARG_WITH(malloc,
[  --with-malloc=OPTION    select malloc wrapper to use: native/check/slow @<:@native@:>@], [
  case "$withval" in
  native) AC_DEFINE(USE_GWMEM_NATIVE, 1, [Use native malloc])
          AC_MSG_RESULT(native malloc)
          ;;
  check) AC_DEFINE(USE_GWMEM_CHECK, 1, [Use checking malloc])
         AC_MSG_RESULT(checking malloc)
         ;;
  slow) AC_DEFINE(USE_GWMEM_SLOW, 1, [Use slow malloc])
        AC_MSG_RESULT(slow malloc)
	;;
  *) echo "Unknown malloc wrapper $withval. Oops."; exit 1 ;;
  esac
], [
  case "$mallocdefault" in
  check) AC_DEFINE(USE_GWMEM_CHECK)
     AC_MSG_RESULT(checking malloc)
     ;;
  slow) AC_DEFINE(USE_GWMEM_SLOW)
        AC_MSG_RESULT(slow malloc)
	;;
  *) AC_DEFINE(USE_GWMEM_NATIVE)
          AC_MSG_RESULT(native malloc)
          ;;
  esac
])


dnl Implement --disable-assertions option.

AC_ARG_ENABLE(assertions,
[  --disable-assertions    turn off assertion checking], [
  if test "$enableval" = "no"
  then
    echo disabling assertion checking
    AC_DEFINE(NO_GWASSERT, 1, [No asserts])
  fi
], [
  if test "$assertiondefault" = "no"
  then
    echo disabling assertion checking
    AC_DEFINE(NO_GWASSERT)
  fi
])


dnl Implement the --enable-pam option.

AC_ARG_ENABLE(pam,
[  --enable-pam	          enable PAM authentication @<:@disabled@:>@], [
    if test "$enableval" = "yes"
    then
	AC_CHECK_LIB(pam, pam_end)
	AC_CHECK_LIB(dl,main)
	AC_CHECK_HEADERS(security/pam_appl.h)
	PAMTARGET="pam"
    else
	PAMTARGET="no-pam"
    fi
])
case "$PAMTARGET" in
no-pam) echo PAM authentication is disabled. ;;
pam) echo PAM authentication is enabled. ;;
esac


dnl Implement --enable-debug option.

AC_ARG_ENABLE(debug,
[  --enable-debug	  enable non-reentrant debugging for wmls compiler @<:@disabled@:>@], [
  echo enabling WMLScript compiler debugging
  if test -n "$GCC"; then
    CFLAGS="$CFLAGS -Wall"
  fi
  AC_DEFINE(WS_DEBUG, 1, [WMLScript compiler debugging])
])


dnl Implement --enable-localtime option.

AC_ARG_ENABLE(localtime,
[  --enable-localtime	  log file time stamps in local time, not GMT @<:@enabled@:>@], [
  if test "$enableval" = yes; then
      echo enabling local time
      AC_DEFINE(LOG_TIMESTAMP_LOCALTIME, 1, [enabling local time])
  fi
],[
  echo enabling local time
  AC_DEFINE(LOG_TIMESTAMP_LOCALTIME)
])


dnl --enable-mutex-stats option.

AC_ARG_ENABLE(mutex-stats,
[  --enable-mutex-stats    produce information about lock contention], [
  if test "$enableval" = yes; then
      AC_DEFINE(MUTEX_STATS, 1, [produce information about lock contention])
  fi
])


dnl --disable-cookies option.

AC_ARG_ENABLE(cookies,
[  --disable-cookies       disable cookie support for WSP @<:@enabled@:>@], [
  if test "$enableval" = yes; then
    echo enabling cookies
    AC_DEFINE(ENABLE_COOKIES, 1, [Enable cookies])
  else
    echo disabling cookies
  fi
],[
  echo enabling cookies
  AC_DEFINE(ENABLE_COOKIES, 1, [Enable cookies])
])


dnl --disable-keepalive option.

AC_ARG_ENABLE(keepalive,
[  --disable-keepalive     disable HTTP/1.1 keep-alive support @<:@enabled@:>@], [
  if test "$enableval" = yes; then
    echo enabling HTTP/1.1 keep-alive
    AC_DEFINE(USE_KEEPALIVE, 1, [Enable HTTP Keep-Alive])
  else
    echo disabling HTTP/1.1 keep-alive
  fi
],[
  echo enabling HTTP/1.1 keep-alive
  AC_DEFINE(USE_KEEPALIVE, 1, [Enable HTTP Keep-Alive])
])


dnl Implement --disable-sms option.

AC_ARG_ENABLE(sms,
[  --disable-sms           disables SMS gateway parts in bearerbox], [
  if test "$enableval" = "no"
  then
    echo disabling SMS gateway parts in bearerbox
    AC_DEFINE(NO_SMS, 1, [disabling SMS gateway parts in bearerbox])
  fi
])


dnl Implement the --with-ssl option.

AC_CONFIG_SECTION([Configuring OpenSSL support])
AC_ARG_WITH(ssl,
[  --with-ssl[=DIR]          where to look for OpenSSL libs and header files
                          DIR points to the installation @<:@/usr/local/ssl@:>@],
[ if test -d "$withval"; then
    ssllib="$withval/lib";
    sslinc="$withval/include"
  else
    AC_MSG_ERROR(Unable to find OpenSSL libs and/or directories at $withval)
  fi
])


dnl Implement --enable-ssl option.

AC_MSG_CHECKING([whether to compile with SSL support])
AC_ARG_ENABLE(ssl,
[  --enable-ssl            enable SSL client and server support @<:@enabled@:>@], [
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
  dnl test only if --with-ssl has not been used
  if test "x$ssllib" = "x" && test "x$sslinc" = "x"; then
    for loc in /usr/lib /usr/lib64 /usr/local/ssl/lib /usr/local/openssl/lib; do
      if test -f "$loc/libssl.a" -o -f "$loc/libssl.so" -o -f "$loc/libssl.dylib" ; then
        ssllib="$loc"
      fi
    done
    if test "x$sslib" = "x" && test -n "$GCC"; then
      for lib in libssl.a libssl.so libssl.dylib; do
        name=`$CC -print-file-name=$lib`
        if test -f "$name"; then
          ssllib=`dirname $name`
        fi
      done
    fi
    for loc in /usr/include /usr/local/ssl/include /usr/local/openssl/include; do
      if test -f "$loc/openssl/ssl.h"; then
        sslinc="$loc"
        break
      fi
    done
  fi
  AC_MSG_RESULT(trying $ssllib $sslinc)
fi


dnl Implement the SSL library checking routine.
dnl This will define HAVE_LIBSSL in gw-config.h

if test "x$ssllib" != "x" && test "x$sslinc" != "x"; then
  CFLAGS="$CFLAGS -I$sslinc"
  LIBS="$LIBS -L$ssllib"

  AC_PATH_PROG(OPENSSL, openssl, no)
  if test "$OPENSSL" != "no"; then
    AC_MSG_CHECKING([openssl version])
    openssl_version=`$OPENSSL version | awk '{print $2}'`
    openssl_ver=`echo $openssl_version | sed 's/[[a-zA-Z]]//g'`
    AC_MSG_RESULT([$openssl_version])
  fi
  AC_CHECK_VERSION($openssl_ver, "1.1.0",
  [ CRYPTO_CHECK=HMAC_Update
    SSL_CHECK=OPENSSL_init_ssl
    openssl_modern=yes
  ],[
    CRYPTO_CHECK=CRYPTO_lock
    SSL_CHECK=SSL_library_init
    openssl_modern=no
  ])
  AC_CHECK_LIB(crypto, $CRYPTO_CHECK,
  [ LIBS="$LIBS -lcrypto"
    AC_CHECK_LIB(ssl, $SSL_CHECK,
    [ AC_CHECK_LIB(ssl, SSL_connect)
      AC_CHECK_HEADERS(openssl/x509.h openssl/rsa.h openssl/crypto.h \
                       openssl/pem.h openssl/ssl.h openssl/err.h \
                       openssl/hmac.h)
      dnl OpenSSL 1.1+ is always thread-safe, skip the test
      if test "$openssl_modern" = "yes"; then
        AC_MSG_CHECKING([whether the OpenSSL library is multithread-enabled])
        AC_MSG_RESULT([yes (OpenSSL 1.1+ is always thread-safe)])
        AC_DEFINE([HAVE_LIBSSL], [1], [Do we have libssl])
        LIBS="$LIBS -lssl"
        AC_MSG_CHECKING([whether to compile with SSL support])
        AC_MSG_RESULT(yes)
      else
        AC_MSG_CHECKING(whether the OpenSSL library is multithread-enabled)
        AC_RUN_IFELSE([AC_LANG_SOURCE([[
          #define OPENSSL_THREAD_DEFINES
          #include <openssl/opensslconf.h>
          #include <stdlib.h>
          int main(void) {
          #if defined(THREADS)
            exit(0);
          #elif defined(OPENSSL_THREADS)
            exit(0);
          #else
            exit(1);
          #endif
          }
        ]])], [
          AC_MSG_RESULT(yes)
          AC_DEFINE([HAVE_LIBSSL], [1], [Do we have libssl])
          LIBS="$LIBS -lssl"
          AC_MSG_CHECKING([whether to compile with SSL support])
          AC_MSG_RESULT(yes)
        ], [
          AC_MSG_RESULT(no)
          AC_MSG_ERROR(Either get a multithread-enabled SSL or configure with --disable-ssl)
        ], [AC_MSG_RESULT([cross-compiling; make sure your SSL library is multithread-enabled])]
        )
      fi
    ])
  ])
fi


AC_CONFIG_SECTION([Configuring DB support])


dnl Implement the --with-mysql option. This will set HAVE_MYSQL in gw-config.h
dnl accordingly and enable the usage of the libmysqlclient routines.

AC_MSG_CHECKING([whether to compile with MySQL support])
AC_ARG_WITH(mysql,
[  --with-mysql            enable MySQL storage @<:@disabled@:>@], [
if test "$withval" != yes ; then
    AC_MSG_RESULT(disabled)
else
    dnl Implement the --with-mysql-dir option.
    AC_ARG_WITH(mysql-dir,
    [  --with-mysql-dir=DIR    where to look for MySQL libs and header files
                          DIR points to the installation @<:@/usr/local/mysql@:>@],
    [
        mysqlloc=""
        if test -d "$withval" ; then
        mysqlloc="$withval"
        fi
    ])
    
    AC_MSG_RESULT(searching)
    AC_PATH_PROG(MYSQL_CONFIG, mysql_config, no, [$PATH:$mysqlloc/bin:$mysqlloc])
    dnl check for MySQL 4.x style mysql_config information
    if test "$MYSQL_CONFIG" = "no"; then
        found=""
        for loc in $mysqlloc /usr /usr/local ; do
        if test "x$found" = "x" ; then
            AC_MSG_CHECKING([for MySQL client support in])
            AC_MSG_RESULT($loc)
            AC_CHECK_FILE("$loc/include/mysql/mysql.h",
            [CFLAGS="$CFLAGS -I$loc/include/mysql"; LIBS="$LIBS -L$loc/lib/mysql -lmysqlclient"]; found=1,
            [AC_CHECK_FILE("$loc/include/mysql.h",
                [CFLAGS="$CFLAGS -I$loc/include"; LIBS="$LIBS -L$loc/lib -lmysqlclient"]; found=1
            )]
            )
        fi
        done
        if test "x$found" != "x1" ; then
        AC_MSG_ERROR([Unable to find mysql.h, please provide a --with-mysql-dir=<dir> location])
        fi
    else
        dnl mysql_config found
        AC_MSG_CHECKING([mysql version])
        mysql_version=`$MYSQL_CONFIG --version`
        AC_MSG_RESULT([$mysql_version])
        dnl mysql-4.x style
        MYSQL_LIBS=""
        if $MYSQL_CONFIG --libs_r &>/dev/null ; then
            MYSQL_LIBS=`$MYSQL_CONFIG --libs_r`
            AC_MSG_CHECKING([mysql reentrant libs])
            AC_MSG_RESULT([$MYSQL_LIBS])
            AC_CHECK_LIB(mysqlclient_r, mysql_init, [ LIBS="$LIBS $MYSQL_LIBS" ],
                [ MYSQL_LIBS="" ], [ $MYSQL_LIBS ])
        fi
        if test -z "$MYSQL_LIBS" ; then
            MYSQL_LIBS=`$MYSQL_CONFIG --libs`
            AC_MSG_CHECKING([mysql libs])
            AC_MSG_RESULT([$MYSQL_LIBS])
            AC_CHECK_LIB(mysqlclient, mysql_init, [ LIBS="$LIBS $MYSQL_LIBS" ],
                [AC_MSG_ERROR([Unable to find MySQL client libraries])], [ $MYSQL_LIBS ])
        fi
        AC_MSG_CHECKING([mysql includes])
        dnl mysql-4.x style
        if $MYSQL_CONFIG --include &>/dev/null ; then
            MYSQL_CFLAGS=`$MYSQL_CONFIG --include`
        else
            MYSQL_CFLAGS=`$MYSQL_CONFIG --cflags`
        fi
        CFLAGS="$CFLAGS $MYSQL_CFLAGS"
        AC_MSG_RESULT([$MYSQL_CFLAGS])
    fi
    AC_CHECK_HEADERS(mysql/mysql.h mysql/mysql_version.h)
    AC_CHECK_LIB(mysqlclient_r, mysql_stmt_init, [],
		AC_CHECK_LIB(mysqlclient, mysql_stmt_init, [], [AC_MSG_ERROR([Unable to find MySQL client libraries version >= 4.1])])
    )
    AC_DEFINE(HAVE_MYSQL, 1, [Do we have mysql])
    AC_MSG_CHECKING([whether to compile with MySQL support])
    AC_MSG_RESULT(yes)
    MYSQL="yes"
fi
],[
  AC_MSG_RESULT(disabled)
])


dnl Implement the --with-sdb option. This will set HAVE_SDB in gw-config.h
dnl accordingly and enable the usage of the libsdb routines.

AC_MSG_CHECKING([whether to compile with LibSDB support])
AC_ARG_WITH(sdb,
[  --with-sdb              enable LibSDB storage @<:@disabled@:>@], [
if test "$withval" != yes; then
  AC_MSG_RESULT(disabled)
else
  AC_MSG_RESULT(searching)
  AC_PATH_PROGS(SDB_CONFIG, sdb-config, no)
  if test "$SDB_CONFIG" = "no"; then
    AC_MSG_ERROR(Unable to find sdb-config in path for SDB support)
  else
    AC_MSG_CHECKING([sdb version])
    sdb_version=`$SDB_CONFIG --version`
    AC_MSG_RESULT([$sdb_version])
    CFLAGS="$CFLAGS `$SDB_CONFIG --cflags`"
    AC_CHECK_HEADERS(sdb.h)
    LIBS="$LIBS `$SDB_CONFIG --libs`"
    AC_CHECK_LIB(sdb, sdb_init, 
      [LIBS="$LIBS -lsdb" 
       AC_DEFINE(HAVE_SDB, 1, [do we use libsdb])
       AC_DEFINE_UNQUOTED(LIBSDB_VERSION, "$sdb_version", [libsdb version])
       SDB="yes"], 
      [AC_MSG_ERROR([Unable to find libSDB client libraries])]
    )
  fi
fi
],[
  AC_MSG_RESULT(disabled)
])


dnl Implement the --with-sqlite2 option. This will set HAVE_SQLITE in gw-config.h
dnl accordingly and enable the usage of the libsqlite routines.

AC_MSG_CHECKING([whether to compile with SQLite2 support])
AC_ARG_WITH(sqlite2,
[  --with-sqlite2          enable SQLite2 storage @<:@disabled@:>@], [
if test "$withval" != yes; then
    AC_MSG_RESULT(disabled)
else
    AC_MSG_RESULT(searching)
    AC_CHECK_HEADERS(sqlite.h)
    LIBS="$LIBS -L/usr/local/lib"
    AC_CHECK_LIB(sqlite, sqlite_open, 
      [LIBS="$LIBS -lsqlite" 
       AC_DEFINE(HAVE_SQLITE, 1, [Do we use sqlite])
       SQLITE="yes"], 
      [AC_MSG_ERROR([Unable to find SQLite2 client libraries])]
    )
    AC_PATH_PROGS(SQLITE, sqlite, no)
    if test "$SQLITE" = "no"; then
      AC_MSG_WARN([Unable to find sqlite in path for SQLite2 support])
    else 
      AC_MSG_CHECKING([sqlite version])
      sqlite_version=`$SQLITE -version`
      AC_MSG_RESULT([$sqlite_version])
    fi
fi
],[
  AC_MSG_RESULT(disabled)
])


dnl Implement the --with-sqlite3 option. This will set HAVE_SQLITE3 in gw-config.h
dnl accordingly and enable the usage of the libsqlite3 routines.

AC_MSG_CHECKING([whether to compile with SQLite3 support])
AC_ARG_WITH(sqlite3,
[  --with-sqlite3          enable SQLite3 storage @<:@disabled@:>@], [
if test "$withval" != yes; then
    AC_MSG_RESULT(disabled)
else
    AC_MSG_RESULT(searching)
    AC_CHECK_HEADERS(sqlite3.h)
    LIBS="$LIBS -L/usr/local/lib"
    AC_CHECK_LIB(sqlite3, sqlite3_open, 
      [LIBS="$LIBS -lsqlite3" 
       AC_DEFINE(HAVE_SQLITE3, 1, [Do we use sqlite3])
       SQLITE3="yes"], 
      [AC_MSG_ERROR([Unable to find SQLite3 client libraries])]
    )
    AC_PATH_PROGS(SQLITE3, sqlite3, no)
    if test "$SQLITE3" = "no"; then
      AC_MSG_WARN([Unable to find sqlite3 in path for SQLite3 support])
    else 
      AC_MSG_CHECKING([sqlite3 version])
      sqlite3_version=`$SQLITE3 -version | awk '{print $1}'`
      AC_MSG_RESULT([$sqlite3_version])
    fi
fi
],[
  AC_MSG_RESULT(disabled)
])


dnl Implement the --with-oracle option. This will set HAVE_ORACLE in gw-config.h
dnl accordingly and enable the usage of the OCI routines.

AC_MSG_CHECKING([whether to compile with Oracle support])
AC_ARG_WITH(oracle,
[  --with-oracle           enable ORACLE storage @<:@disabled@:>@],
[
if test "$withval" != yes ; then
    AC_MSG_RESULT(disabled)
else
    AC_MSG_RESULT(searching)
        AC_ARG_WITH(
                oracle-includes,
                [  --with-oracle-includes=DIR  adds oracle include paths],
                [ ORACLE_INCLUDE_PATH="$withval" ],
                [ ORACLE_INCLUDE_PATH="$ORACLE_HOME/rdbms/demo $ORACLE_HOME/rdbms/public" ]
        )
        for a in $ORACLE_INCLUDE_PATH
        do
                CPPFLAGS="$CPPFLAGS -I$a"
        done
        AC_MSG_CHECKING([for oci.h ])
        AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <oci.h>]])],
          [AC_MSG_RESULT(yes)],
          [AC_MSG_ERROR([oci.h not found])])
        CFLAGS="$CFLAGS $CPPFLAGS"

        AC_ARG_WITH(
                oracle-libs,
                [  --with-oracle-libs=DIR  adds oracle library path],
                [ ORACLE_LIB_PATH="$withval" ],
                [ ORACLE_LIB_PATH="$ORACLE_HOME/lib" ]
        )
        for a in $ORACLE_LIB_PATH
        do
                LIBS="$LIBS -L$a"
        done

        dnl Check for Oracle 10g instant client
        AC_CHECK_LIB(clntsh,OCIEnvCreate,[ LIBS="$LIBS -lclntsh" ],
          AC_CHECK_LIB(clntsh,OCIEnvCreate,[ LIBS="$LIBS -lclntsh -lnnz10" ],exit)
        )

        dnl Beware that Oracle 10g doesn't use anymore the libwtcX.so libs,
        dnl so we don't break hard in case they are not present.
        AC_CHECK_LIB(wtc8,wtcstu,[ LIBS="$LIBS -lwtc8" ],
          AC_CHECK_LIB(wtc9,wtcstu,[ LIBS="$LIBS -lwtc9" ], true)
        )

		AC_CHECK_FUNC(OCIPing, [AC_DEFINE(HAVE_OCIPING, 1, [Do we have OCIPing])])

    AC_DEFINE(HAVE_ORACLE, 1, [Do we have Oracle])
    AC_MSG_CHECKING([whether to compile with Oracle support])
    AC_MSG_RESULT(yes)
fi
],
[AC_MSG_RESULT(disabled)])


dnl Implement the --with-pgsql option. This will set HAVE_PGSQL in gw-config.h
dnl accordingly and enable the usage of the libpq routines.

AC_MSG_CHECKING([whether to compile with PostgresSQL support])
AC_ARG_WITH(pgsql,
[  --with-pgsql            enable PostgreSQL storage @<:@disabled@:>@], [
if test "$withval" != yes ; then
    AC_MSG_RESULT(disabled)
else
    dnl Implement the --with-pgsql-dir option.
    AC_ARG_WITH(pgsql-dir,
    [  --with-pgsql-dir=DIR    where to look for PostgreSQL libs and header files
                          DIR points to the installation @<:@/usr/local/pgsql@:>@],
    [
    	pgsqlloc=""
    	if test -d "$withval" ; then
    		pgsqlloc="$withval"
    	fi
    ])
    
    AC_MSG_RESULT(searching)
    AC_PATH_PROG(PGSQL_CONFIG, pg_config, no, [$PATH:$pgsqlloc/bin:$pgsqlloc:/usr/lib/postgresql/bin])
    dnl check for PgSQL >= 7.2 style pg_config information
    if test "$PGSQL_CONFIG" = "no"; then
    	found=""
    	for loc in $pgsqlloc /usr /usr/local ; do
    		if test "x$found" = "x" ; then
    			AC_MSG_CHECKING([for PostgresSQL include files in])
    			AC_MSG_RESULT($loc)
    			AC_CHECK_FILE("$loc/include/postgresql/libpq-fe.h",
            	  [CFLAGS="$CFLAGS -I$loc/include/postgresql"; LIBS="$LIBS -L$loc/lib/postgresql -lpq"]; found=1,
            	  [AC_CHECK_FILE("$loc/include/pgsql/libpq-fe.h",
            	    [CFLAGS="$CFLAGS -I$loc/include/pgsql"; LIBS="$LIBS -L$loc/lib/pgsql -lpq"]; found=1,
            	    [AC_CHECK_FILE("$loc/pgsql/include/libpq-fe.h",
            	    [CFLAGS="$CFLAGS -I$loc/pgsql/include"; LIBS="$LIBS -L$loc/pgsql/lib -lpq"]; found=1,
                  )]
                )])
    		fi
    	done
    	if test "x$found" != "x1" ; then
    		AC_MSG_ERROR([Unable to find libpq-fe.h, please provide a --with-pgsql-dir=<dir> location])
    	fi
    else
    	dnl pg_config found
    	AC_MSG_CHECKING([PostgreSQL version])
    	pgsql_version=`$PGSQL_CONFIG --version | awk '{print $2}'`
    	AC_MSG_RESULT([$pgsql_version])
    	AC_MSG_CHECKING([PostgreSQL libdir])
    	if $PGSQL_CONFIG --libdir &>/dev/null ; then
    		LIBS="$LIBS -L`$PGSQL_CONFIG --libdir`"
    		pg_libdir=`$PGSQL_CONFIG --libdir`
    		AC_MSG_RESULT([$pg_libdir])
        fi
        AC_MSG_CHECKING([PostgreSQL includes])
        if $PGSQL_CONFIG --includedir &>/dev/null ; then
            CFLAGS="$CFLAGS -I`$PGSQL_CONFIG --includedir`"
            CPPFLAGS="$CPPFLAGS -I`$PGSQL_CONFIG --includedir`"
            pg_incdir=`$PGSQL_CONFIG --includedir`
            AC_MSG_RESULT([$pg_incdir])
        fi
    fi
    
    AC_CHECK_HEADERS(libpq-fe.h)
    AC_CHECK_LIB(pq, PQconnectdb, [])
    AC_DEFINE(HAVE_PGSQL, 1, [Do we have PGSQL])
    AC_MSG_CHECKING([whether to compile with PostgreSQL support])
    AC_MSG_RESULT(yes)
    PGSQL="yes"
fi
],[
  AC_MSG_RESULT(disabled)
])


dnl Implement the --with-redis option. This will set HAVE_REDIS in gw-config.h
dnl accordingly and enable the usage of the hiredis routines.

AC_MSG_CHECKING([whether to compile with Redis support])
AC_ARG_WITH(redis,
[  --with-redis            enable Redis storage @<:@disabled@:>@], [
if test "$withval" != yes ; then
    AC_MSG_RESULT(disabled)
else
    dnl Implement the --with-redis-dir option.
    AC_ARG_WITH(redis-dir,
    [  --with-redis-dir=DIR    where to look for Redis libs and header files
                          DIR points to the installation @<:@/usr/local/redis@:>@],
    [
        redisloc=""
        if test -d "$withval" ; then
        redisloc="$withval"
        fi
    ])
    
    AC_MSG_RESULT(searching)
    AC_PATH_PROG(REDIS_CONFIG, redis_config, no, [$PATH:$redisloc/bin:$redisloc])
    if test "$REDIS_CONFIG" = "no"; then
        found=""
        for loc in $redisloc /usr /usr/local ; do
            if test "x$found" = "x" ; then
                AC_MSG_CHECKING([for Redis include files in])
                AC_MSG_RESULT($loc)
                AC_CHECK_FILE("$loc/hiredis.h",
                  [CFLAGS="$CFLAGS -I$loc"; LIBS="$LIBS -L$loc -lhiredis"]; found=1,
                  [AC_CHECK_FILE("$loc/include/hiredis/hiredis.h",
                    [CFLAGS="$CFLAGS -I$loc/include/hiredis"; LIBS="$LIBS -L$loc/lib -lhiredis"]; found=1
                  )]
                )
            fi
        done
        if test "x$found" != "x1" ; then
            AC_MSG_ERROR([Unable to find hiredis.h, please provide a --with-redis-dir=<dir> location])
        fi
    else
        dnl redis_config found
        REDIS_LIBS=""
        if test -z "$REDIS_LIBS" ; then
            REDIS_LIBS=`$REDIS_CONFIG --libs`
            AC_MSG_CHECKING([redis libs])
            AC_MSG_RESULT([$REDIS_LIBS])
            AC_CHECK_LIB(hiredis, [ LIBS="$LIBS $REDIS_LIBS" ],
                [AC_MSG_ERROR([Unable to find Redis client libraries])], [ $REDIS_LIBS ])
        fi
        AC_MSG_CHECKING([redis includes])
        if $REDIS_CONFIG --include &>/dev/null ; then
            REDIS_CFLAGS=`$REDIS_CONFIG --include`
        else
            REDIS_CFLAGS=`$REDIS_CONFIG --cflags`
        fi
        CFLAGS="$CFLAGS $REDIS_CFLAGS"
        AC_MSG_RESULT([$REDIS_CFLAGS])
    fi
    AC_CHECK_HEADERS(hiredis/hiredis.h)
    AC_DEFINE(HAVE_REDIS, 1, [Do we have REDIS])
    AC_MSG_CHECKING([whether to compile with Redis support])
    AC_MSG_RESULT(yes)
    REDIS="yes"
fi
],[
  AC_MSG_RESULT(disabled)
])

dnl Checking for FreeTDS Ct-Lib support
AC_MSG_CHECKING([whether to compile with FreeTDS Ct-Lib support])
AC_ARG_WITH(mssql,
[  --with-mssql[=DIR]        enable FreeTDS Ct-Lib storage support @<:@disabled@:>@
                          DIR points to the installation @<:@/usr/local@:>@], [
    if test "$withval" = "yes"; then
        withval=/usr/local
    fi

    if test "$withval" != "no"; then
        if test -f $withval/include/ctpublic.h; then
            MSSQL_INCDIR=$withval/include
            MSSQL_LIBDIR=$withval/lib
        else
            if test -f $withval/include/freetds/ctpublic.h; then
                MSSQL_INCDIR=$withval/include/freetds
                MSSQL_LIBDIR=$withval/lib
            else
                AC_MSG_RESULT(no)
                AC_MSG_ERROR(Invalid FreeTDS directory - unable to find ctpublic.h)
            fi
        fi
        LIBS="$LIBS -L$MSSQL_LIBDIR -lct"
        CFLAGS="$CFLAGS -I$MSSQL_INCDIR"
        AC_DEFINE(HAVE_MSSQL, 1, [Defined to 1])
        AC_MSG_RESULT(yes)
        MSSQL="yes"
        dnl AC_CHECK_FUNC(dlopen, , AC_CHECK_LIB(dl, dlopen, MSSQL_LFLAGS="$MSSQL_LFLAGS -ldl"))
    fi
],[
    AC_MSG_RESULT(disabled)
])

dnl Implement the --with-cassandra option. This will set HAVE_CASS in gw-config.h

AC_MSG_CHECKING([whether to compile with Cassandra support])
AC_ARG_WITH(cassandra,
[  --with-cassandra            enable Cassandra storage @<:@disabled@:>@], [
if test "$withval" != yes ; then
    AC_MSG_RESULT(disabled)
else
    dnl Implement the --with-cassandra-dir option.
    AC_ARG_WITH(cassandra-dir,
    [  --with-cassandra-dir=DIR    where to look for Cassandra libs and header files
                          DIR points to the installation @<:@/usr/local@:>@],
    [
        cassloc=""
        if test -d "$withval" ; then
        cassloc="$withval"
        fi
    ])
    
    AC_MSG_RESULT(searching)
    found=""
    for loc in $cassloc /usr /usr/local ; do
        if test "x$found" = "x" ; then
            AC_MSG_CHECKING([for Cassandra include files in])
            AC_MSG_RESULT($loc)
            AC_CHECK_FILE("$loc/include/cassandra.h",
              [CFLAGS="$CFLAGS -I$loc/include"; LIBS="$LIBS -L$loc/lib -lcassandra"]; found=1
            )
        fi
    done
    if test "x$found" != "x1" ; then
        AC_MSG_ERROR([Unable to find cassandra.h, please provide a --with-cassandra-dir=<dir> location])
    fi
    AC_CHECK_HEADERS(cassandra.h)
    AC_DEFINE(HAVE_CASS, 1, [Do we have CASS])
    AC_MSG_CHECKING([whether to compile with Cassandra support])
    AC_MSG_RESULT(yes)
    CASS="yes"
fi
],[
  AC_MSG_RESULT(disabled)
])


dnl WTLS support removed - no longer maintained
dnl gSOAP support removed - no longer maintained

AH_VERBATIM([PRINTFLIKE], [
/* If we're using GCC, we can get it to check format function arguments. */
#ifdef __GNUC__
    #define PRINTFLIKE(a,b) __attribute__((format(printf, a, b)))
#else
    #define PRINTFLIKE(a, b)
#endif
])

dnl Final Output

AC_CONFIG_SECTION([Generating output files])
AC_CONFIG_FILES([
	Makefile
	gwlib/Makefile
	gwlib/gw_uuid_types.h
	gw/Makefile
	utils/Makefile
	test/Makefile
	checks/Makefile
	benchmarks/Makefile
])
AC_OUTPUT

dnl LICENSE notice

AC_CONFIG_SECTION([License information])
cat <<X
+--------------------------------------------------------------------+
| License:                                                           |
| This software is subject to the Kannel Software License, available |
| in this distribution in the file LICENSE. By continuing this       |
| installation process, you are bound by the terms of this license   |
| agreement. If you do not agree with the terms of this license, you |
| must abort the installation process at this point.                 |
|                                                                    |
|                      The Kannel Group <http://www.kannel.org/>     |
+--------------------------------------------------------------------+

Thank you for using Kannel.
X
