INCLUDES = -I$(top_srcdir)/src/dialogs -I$(top_srcdir)/src/dialogs/config -I$(top_srcdir)/src/dialogs/wizard $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = config.h gsm.h k6opt.h private.h proto.h unproto.h
libgsm_la_LDFLAGS = $(all_libraries)
noinst_LTLIBRARIES = libgsm.la
libgsm_la_SOURCES = add.c code.c debug.c decode.c gsm_create.c\
	gsm_decode.c	gsm_destroy.c gsm_encode.c gsm_explode.c gsm_implode.c\
	gsm_option.c	gsm_print.c long_term.c lpc.c preprocess.c rpe.c\
	short_term.c table.c
