TARGETS	= $(OBJDIR)gallery
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags gtk+-2.0`
CFLAGS	= -Wall -g -O2
LDFLAGS	= `pkg-config --libs glib-2.0`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

gallery_OBJS = $(OBJDIR)gallery.o
gallery_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
gallery_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)gallery: $(gallery_OBJS)
	$(CC) -o $(OBJDIR)gallery $(gallery_OBJS) $(gallery_LDFLAGS)

$(OBJDIR)gallery.o: gallery.c ../config.h
	$(CC) $(gallery_CFLAGS) -o $(OBJDIR)gallery.o -c gallery.c

clean:
	$(RM) -- $(gallery_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)gallery $(DESTDIR)$(BINDIR)/gallery

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/gallery

.PHONY: all clean distclean install uninstall
