#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

SOURCEFILES = \
	os-config.cpp \
	os-config.h \
	iso639.cpp \
	iso639.h \
	clientinterface.cpp \
	connection.cpp \
	dataset.cpp \
	demux.cpp \
	msgpacket.cpp \
	session.cpp \
	thread.cpp \
	packetbuffer.cpp \
	packetbuffermodel.h


noinst_LTLIBRARIES = libxvdrstatic.la

libxvdrstatic_la_SOURCES = \
	$(SOURCEFILES)

if MINGW32
libxvdrstatic_la_LIBADD = \
	-L$(srcdir)/pthreads-win32 -lpthreadGC2 -lws2_32
else
libxvdrstatic_la_LIBADD = \
	$(PTHREAD_LIBS)
endif

libxvdrstatic_la_LIBADD += \
	$(ZLIB_LIBS)


lib_LTLIBRARIES = libxvdr.la

libxvdr_la_SOURCES = \
	$(SOURCEFILES)

if MINGW32
libxvdr_la_LIBADD = \
	-L$(srcdir)/pthreads-win32 -lpthreadGC2 -lws2_32
else
libxvdr_la_LIBADD = \
	$(PTHREAD_LIBS)
endif

libxvdr_la_LIBADD += \
	$(ZLIB_LIBS)

libxvdr_la_LDFLAGS = \
	-avoid-version


AM_CPPFLAGS = \
	$(ARCH_DEFINES)

INCLUDES = -I$(srcdir)/../include

install:

install-lib: install-exec-am

uninstall:

uninstall-lib: uninstall-exec-am
