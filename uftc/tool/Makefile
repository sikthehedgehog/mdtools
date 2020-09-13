CFLAGS:=$(CFLAGS) -Wall -O3 -s

.PHONY: all
all: uftc

uftc: main.o compress.o decompress.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h compress.h decompress.h
compress.o: compress.c main.h
decompress.o: decompress.c main.h

.PHONY: clean
clean:
	-rm -rf uftc
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
