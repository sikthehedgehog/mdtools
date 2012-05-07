CFLAGS=-Wall -O3 -s -std=c99
LDFLAGS=-lpicel -lpng15 -lz

.PHONY: all
all: mdtiler

mdtiler: main.o tiles.o batch.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.c main.h tiles.h batch.h
tiles.o: tiles.c main.h
batch.o: batch.c main.h tiles.h

.PHONY: clean
clean:
	-rm -rf mdtiler
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
