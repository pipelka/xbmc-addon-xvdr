#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

LIBS   =-ldl
LIBDIR = ../../addons/pvr.vdr.xvdr
LIB    = ../../addons/pvr.vdr.xvdr/XBMC_VDR_xvdr.pvr

lib_LTLIBRARIES = libxvdraddon.la

libxvdraddon_la_SOURCES = \
	XBMCAddon.cpp \
	XBMCAddon.h \
	XBMCCallbacks.cpp \
	XBMCCallbacks.h \
	XBMCChannelScan.cpp \
	XBMCChannelScan.h

libxvdraddon_la_LDFLAGS = \
	-avoid-version

libxvdraddon_la_LIBADD = \
	../libxvdr/src/libxvdrstatic.la
	-lpthread \
	$(ZLIB_LIBS)

CPPFLAGS = \
	$(ARCH_DEFINES)

INCLUDES = \
	-I. -I$(top_srcdir)/src/libxvdr/include -I$(top_srcdir) -I$(srcdir)/include -DUSE_DEMUX -D__STDC_CONSTANT_MACROS -D_LINUX

EXTRA_DIST = \
	include \
	project

XBMC_VDR_xvdr.pvr: libxvdraddon.la
	cp -f .libs/libxvdraddon.so XBMC_VDR_xvdr.pvr
	$(STRIP) -x XBMC_VDR_xvdr.pvr

$(LIB): XBMC_VDR_xvdr.pvr
	cp -f XBMC_VDR_xvdr.pvr $(LIB)

all: $(LIB)

clean-local:
	-rm -f $(LIB) XBMC_VDR_xvdr.pvr

install:
