INCLUDES = -I$(top_srcdir)/src/codecs -I$(top_srcdir)/src/dialogs \
	-I$(top_srcdir)/src/dialogs/config -I$(top_srcdir)/src/dialogs/wizard -I$(top_srcdir)/src/rtp \
	-I$(top_srcdir)/src/rtp/jrtplib $(all_includes)
METASOURCES = AUTO
librtp_la_LDFLAGS = $(all_libraries)
noinst_LTLIBRARIES = librtp.la

librtp_la_SOURCES = rtpvideo.cpp rtpaudio.cpp

SUBDIRS = jrtplib
librtp_la_LIBADD = $(top_builddir)/src/rtp/jrtplib/libjrtp.la
AM_CXXFLAGS = -DRTPDEBUG
