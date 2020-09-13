CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: vgi2eif

vgi2eif: main.o vgi.o eif.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h vgi.h eif.h
vgi.o: vgi.c main.h
eif.o: eif.c main.h

.PHONY: clean
clean:
	rm -rf vgi2eif
	rm -rf *.o

.PHONY: love
love:
	@echo not war?
