TARGETS	= gallery
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags glib-2.0`
CFLAGS	= -Wall -g -O2
LDFLAGS	= `pkg-config --libs glib-2.0`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -p
INSTALL	= install


all: $(TARGETS)

gallery_OBJS = gallery.o
gallery_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
gallery_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

gallery: $(gallery_OBJS)
	$(CC) -o gallery $(gallery_OBJS) $(gallery_LDFLAGS)

gallery.o: gallery.c
	$(CC) $(gallery_CFLAGS) -c gallery.c

clean:
	$(RM) -- $(gallery_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 -- gallery $(DESTDIR)$(BINDIR)/gallery

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/gallery

.PHONY: all clean distclean install uninstall
