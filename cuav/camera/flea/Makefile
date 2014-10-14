CC=gcc
INCLUDE = $(shell python-config --includes) -Iinclude -I/usr/include/flycapture
LIBS = -L../../lib/C -lflycapture-c${D} -L../../lib -lflycapture${D} $(shell python-config --libs)
CFLAGS=-Wall -std=gnu99 $(INCLUDE)

all:
	$(CC) $(CFLAGS) fleatest_lib.c fleatest.c $(LIBS) -o fleatest 

flea.so:
	$(CC) $(CFLAGS) -fPIC -shared fleatest_lib.c fleatest_py.c $(LIBS) -o flea.so


clean:
	rm -f *.o 
