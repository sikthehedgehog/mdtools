CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: tfi2eif

tfi2eif: main.o tfi.o eif.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h tfi.h eif.h
tfi.o: tfi.c main.h
eif.o: eif.c main.h

.PHONY: clean
clean:
	rm -rf tfi2eif
	rm -rf *.o

.PHONY: love
love:
	@echo not war?
