ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README.md
include_HEADERS = cquel.h

EXTRA_DIST = m4/NOTES

lib_LTLIBRARIES = libcquel.la
libcquel_la_SOURCES = cquel.c
libcquel_la_CFLAGS = -Wall -Wextra -std=c11 `pkg-config --libs --cflags icu-uc icu-io` `mysql_config --cflags --libs`

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
