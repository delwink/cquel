ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README.md
licensedir = $(datadir)/licenses/$(PACKAGE_NAME)
dist_license_DATA = COPYING
pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = cquel.pc
include_HEADERS = cquel.h

lib_LTLIBRARIES = libcquel.la
libcquel_la_LDFLAGS = -version-info 0:2:0
libcquel_la_SOURCES = cquel.c
libcquel_la_CFLAGS = -Wall -Wextra -std=c11 `pkg-config --libs --cflags icu-uc icu-io` `mysql_config --cflags --libs`

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
