# Project: packPNM
# Makefile created by Matthias Stirner 01/30/2012
# Working with GCC 4.6.0

CC      = gcc
CPP     = g++
RC      = windres -O coff
RM      = del
CFLAGS  = -DDEV_BUILD -I. -O3 -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer
LDFLAGS = -s -static-libgcc -static-libstdc++
DEPS    = aricoder.h bitops.h ppnmtbl.h ppnmbitlen.h Makefile
OBJ     = aricoder.o bitops.o packpnm.o
RES     = icons.res
BIN     = packPNM

%.o: %.cpp $(DEPS)
	$(CPP) -c -o $@ $< $(CFLAGS)
	
%.res: %.rc
	$(RC) $< $@
	
$(BIN): $(OBJ) $(RES)
	$(CPP) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o
