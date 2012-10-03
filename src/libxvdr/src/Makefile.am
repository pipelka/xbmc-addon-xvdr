#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

SOURCEFILES = \
	os-config.cpp \
	os-config.h \
	avcodec.h \
	iso639.cpp \
	iso639.h \
	requestpacket.cpp \
	clientinterface.cpp \
	connection.cpp \
	dataset.cpp \
	demux.cpp \
	responsepacket.cpp \
	session.cpp \
	thread.cpp


noinst_LTLIBRARIES = libxvdrstatic.la

libxvdrstatic_la_SOURCES = \
	$(SOURCEFILES)

libxvdrstatic_la_LIBADD = \
	-lpthread \
	$(ZLIB_LIBS)


lib_LTLIBRARIES = libxvdr.la

libxvdr_la_SOURCES = \
	$(SOURCEFILES)

libxvdr_la_LIBADD = \
	-lpthread \
	$(ZLIB_LIBS)

libxvdr_la_LDFLAGS = \
	-avoid-version


CPPFLAGS = \
	$(ARCH_DEFINES)

INCLUDES = -I$(srcdir)/../include -D__STDC_CONSTANT_MACROS

install:

install-lib: install-exec-am

uninstall:

uninstall-lib: uninstall-exec-am
