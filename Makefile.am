DATE = $(shell date +%Y%m%d)

SUBDIRS = \
	addons \
	src

DIST_SUBDIRS = \
	$(SUBDIRS) \
	data

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

repo: all dist-zip
	make -C data all
	rm -Rf addon-repository
	mkdir -p addon-repository/pvr.vdr.xvdr
	cp pvr.vdr.xvdr-$(VERSION).zip addon-repository/pvr.vdr.xvdr
	cp addons/pvr.vdr.xvdr/icon.png addon-repository/pvr.vdr.xvdr
	cp data/addons.xml addon-repository
	cp data/addons.xml.md5 addon-repository

repo-daily: all dist-zip-daily
	make -C data all
	rm -Rf addon-repository
	mkdir -p addon-repository/pvr.vdr.xvdr
	cp pvr.vdr.xvdr-$(VERSION).$(DATE).zip addon-repository/pvr.vdr.xvdr
	cp addons/pvr.vdr.xvdr/icon.png addon-repository/pvr.vdr.xvdr
	cat data/addons.xml | sed "s/$(VERSION)/$(VERSION).$(DATE)/g" > addon-repository/addons.xml
	cd addon-repository && md5sum addons.xml > addons.xml.md5

release-repo-daily: repo-daily
	cp -r addon-repository/* ~/Dropbox/Public/xbmc-addon-xvdr
