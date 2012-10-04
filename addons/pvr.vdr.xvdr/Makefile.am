if MINGW32
ADDONNAME = XBMC_VDR_xvdr_WIN32.pvr
else
ADDONNAME = XBMC_VDR_xvdr.pvr
endif

addondir = $(prefix)/addons/pvr.vdr.xvdr

addon_DATA = \
	addon.xml \
	$(ADDONNAME) \
	icon.png

$(ADDONNAME):
	make -C ../../src

install-data-hook: $(ADDONNAME)
	$(STRIP) -x -s $(ADDONNAME)
	-cp -Rf $(srcdir)/resources $(DESTDIR)$(addondir)
	chmod -R +w $(DESTDIR)$(addondir)/resources

uninstall-hook:
	rm -Rf $(DESTDIR)$(addondir)/resources

EXTRA_DIST = \
	resources \
	addon.xml \
	icon.png
