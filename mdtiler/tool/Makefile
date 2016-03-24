CFLAGS=-Wall -O3 -s -std=c99
LDFLAGS=-lpng16 -lz

.PHONY: all
all: mdtiler

mdtiler: main.o tiles.o batch.o bitmap.o map.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.c main.h tiles.h batch.h bitmap.h
tiles.o: tiles.c main.h bitmap.h
batch.o: batch.c main.h tiles.h bitmap.h
bitmap.o: bitmap.c bitmap.h
map.o: map.c main.h bitmap.h tiles.h

.PHONY: clean
clean:
	-rm -rf mdtiler
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
