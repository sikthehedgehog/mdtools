CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: headgen

headgen: main.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h

.PHONY: clean
clean:
	rm -rf headgen
	rm -rf *.o

.PHONY: love
love:
	@echo not war?
