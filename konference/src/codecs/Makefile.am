INCLUDES = -I$(top_srcdir)/src/dialogs -I$(top_srcdir)/src/dialogs/config -I$(top_srcdir)/src/dialogs/wizard $(all_includes)
METASOURCES = AUTO

noinst_LTLIBRARIES = libcodecs.la
noinst_HEADERS = h263.h g711.h codecbase.h gsmcodec.h speexcodec.h
libcodecs_la_SOURCES = h263.cpp g711.cpp g711tabs.cpp codecbase.cpp \
	gsmcodec.cpp speexcodec.cpp
SUBDIRS = gsm

libcodecs_la_LIBADD = $(top_builddir)/src/codecs/gsm/libgsm.la
