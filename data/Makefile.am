all: addons.xml.md5

addons.xml.md5: addons.xml
	md5sum addons.xml > addons.xml.md5

clean:
	rm -f addons.xml
	rm -f addons.xml.md5

EXTRA_DIST = \
	addons.xml.in
