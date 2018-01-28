CFLAGS=-Wall -O3 -s -std=c99
LDFLAGS=-lpng16 -lz

.PHONY: all
all: mdtiler

mdtiler: main.o tiles.o batch.o bitmap.o map.o sprite.o offset.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.c main.h tiles.h batch.h bitmap.h
tiles.o: tiles.c main.h bitmap.h tiles.h
batch.o: batch.c main.h bitmap.h offset.h map.h sprite.h tiles.h
bitmap.o: bitmap.c bitmap.h
map.o: map.c main.h bitmap.h offset.h tiles.h
sprite.o: sprite.c main.h offset.h sprite.h tiles.h
offset.o: offset.c offset.h

.PHONY: clean
clean:
	-rm -rf mdtiler
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
