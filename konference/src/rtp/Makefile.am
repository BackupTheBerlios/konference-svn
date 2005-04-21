INCLUDES = -I$(top_srcdir)/src/dialogs -I$(top_srcdir)/src/dialogs/config \
	-I$(top_srcdir)/src/dialogs/wizard -I$(top_srcdir)/src/rtp -I$(top_srcdir)/src/codecs $(all_includes)
METASOURCES = AUTO
librtp_la_LDFLAGS = $(all_libraries)
noinst_LTLIBRARIES = librtp.la
noinst_HEADERS = jitter.h
librtp_la_SOURCES = jitter.cpp rtpbase.cpp rtpvideo.cpp rtpaudio.cpp
