CFLAGS:=$(CFLAGS) -Wall -O3 -s -std=c99

.PHONY: all
all: midi2esf

midi2esf: main.o batch.o event.o midi.o echo.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h batch.h event.h
batch.o: batch.c main.h echo.h event.h midi.h
event.o: event.c echo.h event.h
midi.o: midi.c main.h event.h midi.h
echo.o: echo.c main.h echo.h event.h

.PHONY: clean
clean:
	-rm -rf midi2esf
	-rm -rf *.o

.PHONY: love
love:
	@echo not war?
