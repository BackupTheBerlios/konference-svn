INCLUDES = $(all_includes) $(KONFERENCE_INCLUDES)
METASOURCES = AUTO

libdlg_la_LDFLAGS = $(all_libraries)
noinst_LTLIBRARIES =  libdlg.la



libdlg_la_SOURCES = dummy.cpp
SUBDIRS = config wizard
libdlg_la_LIBADD = $(top_builddir)/src/dialogs/wizard/libwizard.la\
	$(top_builddir)/src/dialogs/config/libconfigdlg.la
