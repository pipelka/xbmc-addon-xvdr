SUBDIRS = \
	src \
	addons

EXTRA_DIST = \
	include

dist-zip: all
	cd addons && zip -9 -r ../pvr.vdr.xvdr-$(VERSION).zip pvr.vdr.xvdr --exclude \*.am \*.in \*Makefile \*.gitignore

