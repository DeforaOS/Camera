TARGETS	= $(OBJDIR)camera
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libDesktop` -lintl
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

camera_OBJS = $(OBJDIR)camera.o $(OBJDIR)overlay.o $(OBJDIR)window.o $(OBJDIR)main.o
camera_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
camera_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)camera: $(camera_OBJS)
	$(CC) -o $(OBJDIR)camera $(camera_OBJS) $(camera_LDFLAGS)

$(OBJDIR)camera.o: camera.c overlay.h camera.h ../config.h
	$(CC) $(camera_CFLAGS) -o $(OBJDIR)camera.o -c camera.c

$(OBJDIR)overlay.o: overlay.c overlay.h
	$(CC) $(camera_CFLAGS) -o $(OBJDIR)overlay.o -c overlay.c

$(OBJDIR)window.o: window.c camera.h window.h
	$(CC) $(camera_CFLAGS) -o $(OBJDIR)window.o -c window.c

$(OBJDIR)main.o: main.c camera.h window.h ../config.h
	$(CC) $(camera_CFLAGS) -o $(OBJDIR)main.o -c main.c

clean:
	$(RM) -- $(camera_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)camera $(DESTDIR)$(BINDIR)/camera

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/camera

.PHONY: all clean distclean install uninstall
