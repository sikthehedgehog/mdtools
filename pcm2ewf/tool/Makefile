CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: pcm2ewf

pcm2ewf: main.o pcm.o ewf.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h pcm.h ewf.h
pcm.o: pcm.c main.h
ewf.o: ewf.c main.h

.PHONY: clean
clean:
	rm -rf pcm2ewf
	rm -rf *.o

.PHONY: love
love:
	@echo not war?
