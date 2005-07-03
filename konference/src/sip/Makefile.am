INCLUDES = -I$(top_srcdir)/src/dialogs -I$(top_srcdir)/src/dialogs/config -I$(top_srcdir)/src/dialogs/wizard $(all_includes)
METASOURCES = AUTO
libsip_la_LDFLAGS = $(all_libraries)
noinst_LTLIBRARIES = libsip.la
libsip_la_SOURCES = sipfsm.cpp md5digest.cpp sipmsg.cpp sipcallid.cpp \
			sipsdp.cpp sipxpidf.cpp sipurl.cpp sipthread.cpp sipcontainer.cpp sipcall.cpp \
	sipfsmbase.cpp sipregistration.cpp
noinst_HEADERS = sipfsm.h md5digest.h sipmsg.h sipcallid.h sipsdp.h sipxpidf.h \
	sipurl.h sipthread.h sipcontainer.h sipcall.h sipfsmbase.h sipregistration.h \
	definitions.h
