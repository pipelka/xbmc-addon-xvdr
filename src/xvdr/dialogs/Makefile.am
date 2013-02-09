SOURCEFILES = \
	GUIDialogBase.cpp \
	GUIDialogBase.h \
	GUIDialogChannelScanner.cpp \
	GUIDialogChannelScanner.h \
	GUIDialogOk.cpp \
	GUIDialogOk.h \
	GUIDialogYesNo.cpp \
	GUIDialogYesNo.h

noinst_LTLIBRARIES = libxvdrdialogs.la

libxvdrdialogs_la_SOURCES = \
	$(SOURCEFILES)

AM_CPPFLAGS = \
	$(ARCH_DEFINES)

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/libxvdr/include \
	-I$(srcdir)/.. \
	-I$(srcdir)/../include
