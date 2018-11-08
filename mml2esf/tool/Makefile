CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: mml2esf

mml2esf: main.o mml.o esf.o stream.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.c mml.h esf.h
mml.o: mml.c stream.h
esf.o: esf.c stream.h
stream.o: stream.c stream.h

.PHONY: clean
clean:
	-rm -rf mml2esf
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
