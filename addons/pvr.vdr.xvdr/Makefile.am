addondir = $(prefix)/addons/pvr.vdr.xvdr

addon_DATA = \
	addon.xml \
	XBMC_VDR_xvdr.pvr \
	icon.png

XBMC_VDR_xvdr.pvr:
	make -C ../../src

install-data-hook: XBMC_VDR_xvdr.pvr
	$(STRIP) -x -s XBMC_VDR_xvdr.pvr
	-cp -Rf $(srcdir)/resources $(DESTDIR)$(addondir)
	chmod -R +w $(DESTDIR)$(addondir)/resources

uninstall-hook:
	rm -Rf $(DESTDIR)$(addondir)/resources

EXTRA_DIST = \
	resources \
	addon.xml \
	icon.png
