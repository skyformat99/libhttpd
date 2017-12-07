#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libhttpd], [0.1.0], [https://github.com/zhoukk/libhttpd/issues])
AC_SUBST(LIBHTTPD_ABI, [0:1:0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([libhttpd.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_AR

AC_PROG_LIBTOOL
LT_INIT
# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gethostname gettimeofday memset select socket strchr strdup strerror strndup strtol])

AC_CHECK_PROG(PKG_CONFIG, pkg-config, yes)
AM_CONDITIONAL([HAVE_PKG_CONFIG], [test "x$PKG_CONFIG" != "x"])
AS_IF([test "x$PKG_CONFIG" != "x"], [
       AC_CONFIG_FILES([libhttpd.pc])
])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
