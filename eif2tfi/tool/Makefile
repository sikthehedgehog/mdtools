CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: eif2tfi

eif2tfi: main.o tfi.o eif.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h tfi.h eif.h
tfi.o: tfi.c main.h
eif.o: eif.c main.h

.PHONY: clean
clean:
	rm -rf eif2tfi
	rm -rf *.o

.PHONY: love
love:
	@echo not war?
