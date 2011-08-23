#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

LIBS   =-ldl
LIBDIR = ../../addons/pvr.vdr.xvdr
LIB    = ../../addons/pvr.vdr.xvdr/XBMC_VDR_xvdr.pvr

noinst_LTLIBRARIES = libxvdr.la

libxvdr_la_SOURCES = \
	vnsicommand.h \
	client.cpp \
	VNSIChannelScan.cpp \
	VNSIData.cpp \
	VNSIDemux.cpp \
	VNSIRecording.cpp \
	VNSISession.cpp \
	requestpacket.cpp \
	responsepacket.cpp \
	thread.cpp \
	tools.cpp

libxvdr_la_LDFLAGS = \
	-avoid-version

libxvdr_la_LIBADD = \
	-lpthread \
	../libTcpSocket/libTcpSocket.la

INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../include -DUSE_DEMUX -D__STDC_CONSTANT_MACROS -D_LINUX

XBMC_VDR_xvdr.pvr: libxvdr.la
	cp -f .libs/libxvdr.so XBMC_VDR_xvdr.pvr
	$(STRIP) -s -x XBMC_VDR_xvdr.pvr

$(LIB): XBMC_VDR_xvdr.pvr
	cp -f XBMC_VDR_xvdr.pvr $(LIB)

all: $(LIB)

clean-local:
	-rm -f $(LIB) XBMC_VDR_xvdr.pvr
