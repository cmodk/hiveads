#!/bin/bash
PROJECT=hiveads
WD=/tmp/$PROJECT
rm -rf $WD
mkdir -p $WD
cd $WD

git clone git@github.com:cmodk/hiveads.git
cd hiveads
git submodule update --init
source version.sh

BUILD_NUMBER=$((BUILD_NUMBER+1))

echo "Creating package $MAJOR.$MINOR-$BUILD_NUMBER.$MINOR-$BUILD_NUMBER.$MINOR-$BUILD_NUMBER.$MINOR-$BUILD_NUMBER"

./autogen.sh
make clean || exit
make -j4 || exit
make DESTDIR=`pwd`/debian install || exit
sed -i "s/Version.*/Version:\ $MAJOR.$MINOR-$BUILD_NUMBER/" debian/DEBIAN/control
dpkg-deb --build debian || exit
reprepro -b /srv/webserver_priv/repos/apt/debian includedeb stretch debian.deb || exit

echo "MAJOR=$MAJOR" > version.sh
echo "MINOR=$MINOR" >> version.sh
echo "BUILD_NUMBER=$BUILD_NUMBER" >> version.sh

git add version.sh
git commit -m "Generated release $MAJOR-$MINOR-$BUILD"


