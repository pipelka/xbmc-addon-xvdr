addondir = $(prefix)/addons/pvr.vdr.xvdr

addon_DATA = \
	XBMC_VDR_xvdr.pvr \
	icon.png \
	pthreadVC2d.dll \
	pthreadVC2.dll

install-data-hook: XBMC_VDR_xvdr.pvr
	cp -Rf $(srcdir)/resources $(DESTDIR)$(addondir)
