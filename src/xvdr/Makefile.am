#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

LIBS   =-ldl
LIBDIR = ../../addons/pvr.vdr.xvdr
LIB    = ../../addons/pvr.vdr.xvdr/XBMC_VDR_xvdr.pvr

lib_LTLIBRARIES = libxvdr.la

libxvdr_la_SOURCES = \
	codecids.h \
	xvdrcommand.h \
	client.cpp \
	client.h \
	iso639.cpp \
	iso639.h \
	XVDRChannelScan.cpp \
	XVDRChannelScan.h \
	XVDRData.cpp \
	XVDRData.h \
	XVDRDemux.cpp \
	XVDRDemux.h \
	XVDRRecording.cpp \
	XVDRRecording.h \
	XVDRSession.cpp \
	XVDRSession.h \
	requestpacket.cpp \
	requestpacket.h \
	responsepacket.cpp \
	responsepacket.h \
	thread.cpp \
	thread.h \
	tools.cpp \
	tools.h

libxvdr_la_LDFLAGS = \
	-avoid-version

libxvdr_la_LIBADD = \
	-lpthread \
	../libTcpSocket/libTcpSocket.la \
	$(ZLIB_LIBS)

CPPFLAGS = \
	$(ARCH_DEFINES)

INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../include -DUSE_DEMUX -D__STDC_CONSTANT_MACROS -D_LINUX

EXTRA_DIST = \
	project

XBMC_VDR_xvdr.pvr: libxvdr.la
	cp -f .libs/libxvdr.so XBMC_VDR_xvdr.pvr
	$(STRIP) -x XBMC_VDR_xvdr.pvr

$(LIB): XBMC_VDR_xvdr.pvr
	cp -f XBMC_VDR_xvdr.pvr $(LIB)

all: $(LIB)

clean-local:
	-rm -f $(LIB) XBMC_VDR_xvdr.pvr

install:

