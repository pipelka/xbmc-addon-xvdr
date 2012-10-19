
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := XBMC_VDR_xvdr.so
LOCAL_SRC_FILES := \
	../src/libxvdr/src/clientinterface.cpp \
	../src/libxvdr/src/connection.cpp \
	../src/libxvdr/src/dataset.cpp \
	../src/libxvdr/src/demux.cpp \
	../src/libxvdr/src/iso639.cpp \
	../src/libxvdr/src/os-config.cpp \
	../src/libxvdr/src/requestpacket.cpp \
	../src/libxvdr/src/responsepacket.cpp \
	../src/libxvdr/src/session.cpp \
	../src/libxvdr/src/thread.cpp \
	../src/xvdr/XBMCAddon.cpp \
	../src/xvdr/XBMCCallbacks.cpp \
	../src/xvdr/XBMCChannelScan.cpp \
	../src/xvdr/XBMCSettings.cpp

LOCAL_CFLAGS := \
	-I$(LOCAL_PATH)/../src/libxvdr/include \
	-I$(LOCAL_PATH)/../src/xvdr/include \
	-I$(LOCAL_PATH)/.. \
	-DHAVE_ZLIB=1 -DUSE_DEMUX=1

LOCAL_LDLIBS := -lz

include $(BUILD_SHARED_LIBRARY)
