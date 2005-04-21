INCLUDES = -I$(top_srcdir)/src/dialogs -I$(top_srcdir)/src/dialogs/config 	-I$(top_srcdir)/src/dialogs/wizard $(all_includes)
METASOURCES = AUTO

noinst_LTLIBRARIES = libaudio.la
noinst_HEADERS = audiobase.h alsa.h oss.h audioarts.h
libaudio_la_SOURCES = audiobase.cpp alsa.cpp oss.cpp audioarts.cpp
libaudio_la_LIBADD = -lartsc -lasound
