CFLAGS=-Wall -O3 -s

.PHONY: all
all: slz

slz: main.o compress.o decompress.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h compress.h decompress.h
compress.o: compress.c main.h
decompress.o: decompress.c main.h

.PHONY: clean
clean:
	-rm -rf slz
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
