REM 12-06-17 Gerhard Seelmann
REM 13-12-07 Matthias Stirner
REM

REM Set path to 'gcc'(V4), 'make', and 'upx' (V3.08 or above)
REM Use short file names (w/o spaces), otherwise 'g++' failed for me

@ECHO OFF

SET GCC_BIN_DIR=C:\MinGW\bin
SET UPX_BIN_DIR=C:\UPX
SET OLDPATH=%PATH%
SET PATH=%GCC_BIN_DIR%;%UPX_BIN_DIR%;%PATH%

CD .\packANY\packANYlib
make -f Makefile_lib
make -f Makefile_lib clean
make -f Makefile_lib_Os
make -f Makefile_lib_Os clean

CD ..\..\packARC
MOVE ..\packANY\packANYlib\packANYlib.a .\
MOVE ..\packANY\packANYlib\packANYlib_small.a .\

make -f Makefile_sfx_stub.win
make -f Makefile_sfx_stub.win clean
upx --best --lzma sfxstub.exe

gcc -o sfxstub2h.exe sfxstub2h.c
.\sfxstub2h
make -f Makefile.win
make -f Makefile.win clean
upx --best --lzma packARC.exe

DEL sfxstub.h sfxstub.exe sfxstub2h.exe packANYlib.a packANYlib_small.a

CD ..
MOVE .\packARC\packARC.exe .\

set PATH=%OLDPATH%