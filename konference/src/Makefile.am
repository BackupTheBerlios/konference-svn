# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/src/dialogs -I$(top_srcdir)/src/dialogs/config \
	-I$(top_srcdir)/src/dialogs/wizard $(all_includes)

# these are the headers for your project
noinst_HEADERS = konference.h konference_part.h videowidget.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/konference.pot

KDE_ICON = konference

# this Makefile creates both a KPart application and a KPart
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = konference

# the application source, library search path, and link libraries
konference_SOURCES = main.cpp konference.cpp
konference_LDFLAGS = $(KDE_RPATH) $(all_libraries)
konference_LDADD = $(LIB_KPARTS)

# this is where the desktop file will go 
shelldesktopdir = $(kde_appsdir)/Utilities
shelldesktop_DATA = konference.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir = $(kde_datadir)/konference
shellrc_DATA = konference_shell.rc

#########################################################################
# KPART SECTION
#########################################################################
kde_module_LTLIBRARIES = libkonferencepart.la

# the Part's source, library search path, and link libraries
libkonferencepart_la_SOURCES = konference_part.cpp videowidget.cpp \
	settings.kcfgc
libkonferencepart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkonferencepart_la_LIBADD = $(top_builddir)/src/video/libvideo.la \
	$(top_builddir)/src/audio/libaudio.la $(top_builddir)/src/codecs/libcodecs.la \
	$(top_builddir)/src/dialogs/libdlg.la $(top_builddir)/src/rtp/librtp.la $(top_builddir)/src/sip/libsip.la \
	-lavcodec $(LIB_KPARTS) $(LIB_KFILE)

# this is where the desktop file will go 
partdesktopdir = $(kde_servicesdir)
partdesktop_DATA = konference_part.desktop

# this is where the part's XML-GUI resource file goes
partrcdir = $(kde_datadir)/konferencepart
partrc_DATA = konference_part.rc


partlogodir = $(kde_datadir)/konference
partlogo_DATA = logo.png

#KConfigXT stuff
kde_kcfg_DATA = konference.kcfg

SUBDIRS = dialogs rtp sip codecs audio video
