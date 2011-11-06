DATE = $(shell date +%Y%m%d)

SUBDIRS = \
	src \
	addons

EXTRA_DIST = \
	include

dist-zip: all
	cd addons && zip -9 -r ../pvr.vdr.xvdr-$(VERSION).zip pvr.vdr.xvdr --exclude \*.am \*.in \*Makefile \*.gitignore

dist-zip-daily:
	cp -r addons/pvr.vdr.xvdr .
	cat pvr.vdr.xvdr/addon.xml | sed "s/$(VERSION)/$(VERSION).$(DATE)/g" > pvr.vdr.xvdr/addon.xml.new
	mv pvr.vdr.xvdr/addon.xml.new pvr.vdr.xvdr/addon.xml
	zip -9 -r ./pvr.vdr.xvdr-$(VERSION).$(DATE).zip pvr.vdr.xvdr --exclude \*.am \*.in \*Makefile \*.gitignore
	rm -Rf pvr.vdr.xvdr

dist-git: all
	GIT_REV=`git --no-pager log --abbrev=7 -n 1 --pretty=format:"%h %ci" HEAD | cut -f1 -d' '` ; \
	make VERSION=$$GIT_REV dist
