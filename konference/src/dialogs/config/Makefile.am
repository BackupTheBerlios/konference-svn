INCLUDES = -I$(top_srcdir)/src/ $(all_includes) $(KONFERENCE_INCLUDES)

METASOURCES = AUTO

libconfigdlg_la_LDFLAGS = $(all_libraries)

noinst_LTLIBRARIES = libconfigdlg.la

noinst_HEADERS = configdialog.h configpersonaldlg.h configvideodlg.h \
	configaudiodlg.h configviewdlg.h
libconfigdlg_la_SOURCES = configdialog.cpp configpersonaldlglayout.ui \
	configpersonaldlg.cpp configvideodlglayout.ui configvideodlg.cpp configaudiodlglayout.ui \
	configaudiodlg.cpp configconnectiondlglayout.ui configconnectiondlg.cpp \
	configviewdlglayout.ui configviewdlg.cpp

configdialog.cpp:
	cd $(top_builddir)/src/ && $(MAKE) settings.h
