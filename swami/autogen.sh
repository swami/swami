#!/bin/sh

echo "Initializing Swami build..."
error=0

for prog in aclocal autoheader autoconf automake libtoolize gtkdocize; do
  which $prog 2>&1 >/dev/null || {
    echo "*** The program '$prog' was not found and is required to initialize build."
    exit 1
  }
done

gtkdocize --copy --flavour no-tmpl && aclocal -I . -I m4 && autoheader && autoconf \
  && automake --add-missing -c && libtoolize -c -f \
  || error=1

if [ error = "1" ]; then
  echo "*** An error occurred while initializing the build environment."
  echo "*** Make sure you have recent versions of autoconf, automake, and"
  echo "*** libtool installed when building from CVS or regenerating build."
  exit 1
fi
