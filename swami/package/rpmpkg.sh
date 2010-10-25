#!/bin/sh
# Package a Swami tarball into RPM packages
# Careful with this script!! It needs to be run as root to build RPMs.
# In particular an "rm -rf $TMPDIR" is done

RPMROOT=/usr/src/RPM
PKGDIR=.
TMPDIR=${PKGDIR}/packtmp

if test ! -e "./$0" ; then
    echo "Run this script from within the package directory"
    exit 1
fi

if test -z "$1" -o ! -e "$1" ; then
    echo "Usage: $0 swami-x.xx.x.tar.gz"
    exit 1
fi

if test $LOGNAME != "root" ; then
    echo "Should be run as root to build RPMs"
    exit 1
fi

SWAMIDIR=`basename $1 .tar.gz`

# Unset LINGUAS so all languages will be built
unset LINGUAS

rm -rf ${TMPDIR} && \
mkdir ${TMPDIR} && \

tar -C ${TMPDIR} -xzf $1 && \

cat ${TMPDIR}/${SWAMIDIR}/swami.spec \
    | sed 's|^\./configure.*$|& --disable-alsa-support --with-audiofile'\
' --disable-awe-caching|' \
    >${TMPDIR}/swami-binary-rpm.spec && \

mv ${TMPDIR}/${SWAMIDIR}/swami.spec ${TMPDIR}/swami-binary-rpm.spec \
    ${RPMROOT}/SPECS/ && \

cp $1 ${RPMROOT}/SOURCES/ && \

rpm -bs ${RPMROOT}/SPECS/swami.spec && \
rpm -bb ${RPMROOT}/SPECS/swami-binary-rpm.spec && \

find ${RPMROOT}/RPMS/ -name "swami*.rpm" -exec mv {} ${PKGDIR} ';' && \
find ${RPMROOT}/SRPMS/ -name "swami*.rpm" -exec mv {} ${PKGDIR} ';' && \

rm -rf ${TMPDIR}

exit 0
