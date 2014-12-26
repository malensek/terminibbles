bin=terminibbles
version=1.3
distname=$(bin)-$(version)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LEVELDIR ?= $(PREFIX)/share/$(bin)/levels
MANDIR ?= $(PREFIX)/share/man

CFLAGS += -Wall -lncurses -D'LEVEL_DIR="$(LEVELDIR)"' -D'VERSION="$(version)"'

tnibbles_src=gameboard.c terminibbles.c
tnibbles_obj=$(tnibbles_src:.c=.o)
distrib=$(tnibbles_src) gameboard.h terminibbles.1 levels \
		LICENSE.txt LEVELS.txt README.md Makefile

all: $(bin)

$(bin): $(tnibbles_obj)
	$(CC) $(CFLAGS) $(tnibbles_obj) -o $@

install: all
	install -d $(DESTDIR)/$(BINDIR)
	install $(bin) $(DESTDIR)/$(BINDIR)
	install -d $(DESTDIR)/$(LEVELDIR)
	install -m644 ./levels/* $(DESTDIR)/$(LEVELDIR)
	install -d $(DESTDIR)/$(MANDIR)/man1/
	install terminibbles.1 $(DESTDIR)/$(MANDIR)/man1/

tar: clean
	mkdir ./$(distname)
	cp -r $(distrib) ./$(distname)
	tar -czf $(distname).tar.gz ./$(distname)
	rm -r ./$(distname)

clean:
	rm -f $(tnibbles_obj)
	rm -f makefile.bak
	rm -f $(distname).tar.gz
	rm -f $(bin)

gameboard.o: gameboard.h
terminibbles.o: gameboard.h
