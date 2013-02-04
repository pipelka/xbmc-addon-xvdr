if MINGW32
ADD_LIBS = -L$(srcdir)/../src/pthreads-win32 -lpthreadGC2 -lws2_32
else
ADD_LIBS = $(PTHREAD_LIBS)
endif

noinst_PROGRAMS = \
	ac3analyze \
	demux \
	listener \
	scanner

demux_SOURCES = \
	consoleclient.cpp \
	consoleclient.h \
	demux.cpp

demux_LDADD = \
	../src/libxvdrstatic.la \
	$(ADD_LIBS)

listener_SOURCES = \
	consoleclient.cpp \
	consoleclient.h \
	listener.cpp

listener_LDADD = \
	../src/libxvdrstatic.la \
	$(ADD_LIBS)

scanner_SOURCES = \
	consoleclient.cpp \
	consoleclient.h \
	scanner.cpp

scanner_LDADD = \
	../src/libxvdrstatic.la \
	$(ADD_LIBS)

ac3analyze_SOURCES = \
	consoleclient.cpp \
	consoleclient.h \
	ac3analyze.cpp

ac3analyze_LDADD = \
	../src/libxvdrstatic.la \
	$(ADD_LIBS)

INCLUDES = \
	-I$(srcdir)/../include
