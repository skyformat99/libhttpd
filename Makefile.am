ACLOCAL_AMFLAGS = -I m4

include_HEADERS = libhttpd.h lib/ae.h lib/anet.h lib/fmacros.h lib/zmalloc.h lib/config.h

lib_LTLIBRARIES = libhttpd.la

libhttpd_la_SOURCES = libhttpd.c http_parser.c lib/ae.c lib/anet.c lib/zmalloc.c
libhttpd_la_CFLAGS = -fvisibility=hidden
libhttpd_la_LDFLAGS = -version-info @LIBHTTPD_ABI@

EXTRA_DIST = lib/ae_epoll.c lib/ae_evport.c lib/ae_kqueue.c lib/ae_select.c

bin_PROGRAMS = httpd

httpd_SOURCES = httpd.c
httpd_CFLAGS =
httpd_LDFLAGS =
httpd_LDADD = libhttpd.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libhttpd.pc
