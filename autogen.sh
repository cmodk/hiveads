#!/bin/bash
cur_dir=`pwd`
make clean
make distclean
touch NEWS README AUTHORS ChangeLog compile
rm configure Makefile Makefile.in config.log config.status 

cd $cur_dir
autoreconf --force -v --install ||exit
./configure --host=arm-linux-gnueabihf --prefix=/usr ||exit

