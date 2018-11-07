CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: romfix

romfix: main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.c main.h

.PHONY: clean
clean:
	-rm -rf romfix
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
