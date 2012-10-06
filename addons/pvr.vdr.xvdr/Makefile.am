addondir = $(prefix)/addons/pvr.vdr.xvdr

addon_DATA = \
	addon.xml \
	$(ADDONNAME) \
	icon.png

if MINGW32
addon_DATA += \
	pthreadGC2.dll
endif

$(ADDONNAME):
	make -C ../../src

install-data-hook: $(ADDONNAME)
	-cp -Rf $(srcdir)/resources $(DESTDIR)$(addondir)
	chmod -R +w $(DESTDIR)$(addondir)/resources

uninstall-hook:
	rm -Rf $(DESTDIR)$(addondir)/resources

EXTRA_DIST = \
	resources \
	addon.xml \
	icon.png
