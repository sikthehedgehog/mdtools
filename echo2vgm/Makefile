CFLAGS:=$(CFLAGS) -Wall -O3 -s
OBJECTS=main.o instruments.o stream.o esf.o vgm.o gd3.o util.o

.PHONY: all
.PHONY: clean
.PHONY: love

all: echo2vgm
echo2vgm: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c instruments.h esf.h vgm.h gd3.h
instruments.o: instruments.c instruments.h util.h
instruments.h: util.h
stream.o: stream.c stream.h
esf.o: esf.c instruments.h stream.h esf.h util.h
vgm.o: vgm.c instruments.h stream.h vgm.h gd3.h util.h
gd3.o: gd3.c gd3.h util.h
gd3.h: util.h
util.o: util.c util.h

clean:
	-rm -f echo2vgm
	-rm -f *.o

love:
	echo not war?
