TARGETS	= camera
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libDesktop`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

camera_OBJS = camera.o overlay.o main.o
camera_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
camera_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

camera: $(camera_OBJS)
	$(CC) -o camera $(camera_OBJS) $(camera_LDFLAGS)

camera.o: camera.c overlay.h camera.h
	$(CC) $(camera_CFLAGS) -c camera.c

overlay.o: overlay.c overlay.h
	$(CC) $(camera_CFLAGS) -c overlay.c

main.o: main.c camera.h
	$(CC) $(camera_CFLAGS) -c main.c

clean:
	$(RM) -- $(camera_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 camera $(DESTDIR)$(BINDIR)/camera

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/camera

.PHONY: all clean distclean install uninstall
