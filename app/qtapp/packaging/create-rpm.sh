#!/bin/sh
# This script wil create a RPM package for the Qt GUI of RtkLib
#

RTKLIB_H=../../../src/rtklib.h
VERSION=$(sed -n 's/^#define VER_RTKLIB *"\([^ ]*\)"  *\(.*\) *$/\1/p' $RTKLIB_H)
PATCH_LEVEL=$(sed -n 's/^#define PATCH_LEVEL *"\([^ ]*\)"  *\(.*\) *$/\1/p' $RTKLIB_H)
RPMBUILD=~/rpmbuild

if [ -f Makefile ]; then
	echo Cleaning objects from previous compile
	make -C ../ distclean
fi

# Prepare RPM build directory hierarchy
if [ ! -f $RPMBUILD ]; then
	echo Creating RPMBUILD hierarchy in $RPMBUILD
	mkdir -p $RPMBUILD/SOURCES $RPMBUILD/SPECS
fi

sed -e "s/\$RTKLIB_VERSION/$VERSION/g" -e "s/\$RTKLIB_RELEASE/$PATCH_LEVEL/g" rtklib-qt.spec > $RPMBUILD/SPECS/rtklib-qt.spec

# Prepare a source tarball and move it under $RPMBUILD/SOURCES
echo "Packing sources into rtklib-qt-$VERSION.tar.gz..."
if [ -d /tmp/rtklib-qt-$VERSION ]; then
	rm -rf /tmp/rtklib-qt-$VERSION
fi

mkdir /tmp/rtklib-qt-$VERSION
rsync -aC ../../.. /tmp/rtklib-qt-$VERSION
tar --directory=/tmp -czf /tmp/rtklib-qt-$VERSION.tar.gz rtklib-qt-$VERSION
mv /tmp/rtklib-qt-$VERSION.tar.gz $RPMBUILD/SOURCES

cd $RPMBUILD/SPECS
rpmbuild -ba rtklib-qt.spec
if [ $? == 0 ]; then
	echo Packages created in $RPMBUILD/RPMS
fi
