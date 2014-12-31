ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README.md
pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = cquel.pc

include_HEADERS = cquel.h
lib_LTLIBRARIES = libcquel.la
libcquel_la_LDFLAGS = -version-info 6:0:2
libcquel_la_SOURCES = cquel.c cqstatic.c
libcquel_la_CFLAGS = -Wall -Wextra -std=c11 `mysql_config --cflags --libs`

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
