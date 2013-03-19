bin=terminibbles

prefix ?= /usr/local
bindir ?= $(prefix)/bin
leveldir ?= $(prefix)/share/$(bin)/

CFLAGS=-Wall -lncurses

tnibbles_src=gameboard.c terminibbles.c
tnibbles_obj=$(tnibbles_src:.c=.o)

all: $(bin)

$(bin): $(tnibbles_obj)
	$(CC) $(CFLAGS) $(tnibbles_obj) -o $@

install: all
	install $(bin) $(bindir)
	install -d $(leveldir)
	install -m 644 ./levels/* $(leveldir)

depend:
	makedepend -Y $(tnibbles_src)

tar: clean
	tar -czf $(bin).tar.gz *

clean:
	rm -f $(tnibbles_obj)
	rm -f makefile.bak
	rm -f $(bin).tar.gz
	rm -f $(bin)

gameboard.o: gameboard.h
terminibbles.o: gameboard.h
