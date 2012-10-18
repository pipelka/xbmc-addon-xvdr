#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

LIBS   =
LIBDIR = ../../addons/pvr.vdr.xvdr
LIB    = ../../addons/pvr.vdr.xvdr/$(ADDONNAME)

lib_LTLIBRARIES = libxvdraddon.la

libxvdraddon_la_SOURCES = \
	XBMCAddon.cpp \
	XBMCAddon.h \
	XBMCCallbacks.cpp \
	XBMCCallbacks.h \
	XBMCChannelScan.cpp \
	XBMCChannelScan.h \
	XBMCSettings.cpp \
	XBMCSettings.h

if MINGW32
libxvdraddon_la_SOURCES += \
    $(top_srcdir)/addons/library.xbmc.addon/dlfcn-win32.c
endif

libxvdraddon_la_LDFLAGS = \
	-avoid-version

libxvdraddon_la_LIBADD = \
	../libxvdr/src/libxvdrstatic.la
	$(ZLIB_LIBS)

if MINGW32
libxvdraddon_la_LIBADD += \
	-L$(srcdir)/../libxvdr/src/pthreads-win32 -lpthreadGC2 -lws2_32
else
libxvdraddon_la_LIBADD += \
	$(PTHREAD_LIBS)
endif

CPPFLAGS = \
	$(ARCH_DEFINES)

INCLUDES = \
	-I. \
	-I$(top_srcdir)/src/libxvdr/include \
	-I$(top_srcdir) -I$(srcdir)/include \
	-DUSE_DEMUX -D__STDC_CONSTANT_MACROS -D_LINUX

EXTRA_DIST = \
	include

$(ADDONNAME): libxvdraddon.la
	cp -f .libs/libxvdraddon.$(SOEXT) $(ADDONNAME)

$(LIB): $(ADDONNAME)
	cp -f $(ADDONNAME) $(LIB)

all: $(LIB)

clean-local:
	-rm -f $(LIB) $(ADDONNAME)

install:
