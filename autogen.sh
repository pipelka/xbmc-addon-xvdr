echo "Generating build information ..."
cd `dirname $0`
aclocalinclude="$ACLOCAL_FLAGS"
mkdir -p autotools

echo "Running libtoolize ..."
libtoolize --copy --force --automake || exit 1

echo "Running aclocal $aclocalinclude ..."
aclocal $aclocalinclude || {
    echo
    echo "**Error**: aclocal failed. This may mean that you have not"
    echo "installed all of the packages you need, or you may need to"
    echo "set ACLOCAL_FLAGS to include \"-I \$prefix/share/aclocal\""
    echo "for the prefix where you installed the packages whose"
    echo "macros were not found"
    exit 1
}

echo "Running automake ..."
automake -c -a --foreign || ( echo "***ERROR*** automake failed." ; exit 1 )

echo "Running autoconf ..."
autoconf || ( echo "***ERROR*** autoconf failed." ; exit 1 )

echo
echo "Please run ./configure now."
