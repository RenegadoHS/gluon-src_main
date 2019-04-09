# Project: libparsifal shared library for windows - parsifal.dll
# requirements: mingw in C:\MinGW + in path
# build command: mingw32-make makefile all
# example usage: gcc -Wall -ansi -o el.exe pull1.c xmlreader.c -lparsifal -I"../../include" -L./

CC = gcc.exe
BIN = parsifal.dll
OBJ = ../../../src/bistream.o ../../../src/encoding.o ../../../src/parsifal.o ../../../src/xmlhash.o ../../../src/xmlsbuf.o ../../../src/xmlvect.o ../../../src/xmlpool.o ../../../src/dtdvalid.o
LINKOBJ = ../../../src/bistream.o ../../../src/encoding.o ../../../src/parsifal.o ../../../src/xmlhash.o ../../../src/xmlsbuf.o ../../../src/xmlvect.o ../../../src/xmlpool.o ../../../src/dtdvalid.o
LIBS = -L"C:/MinGW/lib"  
INCS = -I"C:/MinGW/include"  -I"../../../include/libparsifal" 
CFLAGS = $(INCS) -DXMLAPI=__declspec(dllexport) -DDTD_SUPPORT=1 -DMAX_SPEED=1 -DDTDVALID_SUPPORT=1 -Wall -ansi -O3
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	$(RM) $(OBJ) $(BIN)

STATICLIB=parsifal.a

$(BIN): $(LINKOBJ)
	$(CC) -shared -o $(BIN) $(LINKOBJ) -Wl,--out-implib,$(STATICLIB)

../../../src/bistream.o: ../../../src/bistream.c
	$(CC) -c ../../../src/bistream.c -o ../../../src/bistream.o $(CFLAGS)

../../../src/encoding.o: ../../../src/encoding.c
	$(CC) -c ../../../src/encoding.c -o ../../../src/encoding.o $(CFLAGS)

../../../src/parsifal.o: ../../../src/parsifal.c
	$(CC) -c ../../../src/parsifal.c -o ../../../src/parsifal.o $(CFLAGS)

../../../src/xmlhash.o: ../../../src/xmlhash.c
	$(CC) -c ../../../src/xmlhash.c -o ../../../src/xmlhash.o $(CFLAGS)

../../../src/xmlsbuf.o: ../../../src/xmlsbuf.c
	$(CC) -c ../../../src/xmlsbuf.c -o ../../../src/xmlsbuf.o $(CFLAGS)

../../../src/xmlvect.o: ../../../src/xmlvect.c
	$(CC) -c ../../../src/xmlvect.c -o ../../../src/xmlvect.o $(CFLAGS)

../../../src/xmlpool.o: ../../../src/xmlpool.c
	$(CC) -c ../../../src/xmlpool.c -o ../../../src/xmlpool.o $(CFLAGS)

../../../src/dtdvalid.o: ../../../src/dtdvalid.c
	$(CC) -c ../../../src/dtdvalid.c -o ../../../src/dtdvalid.o $(CFLAGS)
