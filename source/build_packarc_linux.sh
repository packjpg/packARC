#!/bin/sh
#
# 12-06-17  Se

cd ./packANY/packANYlib
make -f Makefile_lib_Linux
make -f Makefile_lib_Linux clean
make -f Makefile_lib_Os_Linux
make -f Makefile_lib_Os_Linux clean

cd ../../packARC
cp ../packANY/packANYlib/packANYlib*.a ./
rm ../packANY/packANYlib/packANYlib*.a ./

make -f Makefile_sfx_stub.linux
make -f Makefile_sfx_stub.linux clean
upx --best --lzma sfxstub

gcc -o sfxstub2h sfxstub2h.c
./sfxstub2h
make -f Makefile.linux
make -f Makefile.linux clean
upx --best --lzma packarc

rm sfxstub.h sfxstub sfxstub2h packANYlib.a packANYlib_small.a

cp ./packARC ../packARC.lxe
rm ./packARC
cd ..
