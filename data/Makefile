PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -p
INSTALL	= install
RM	= rm -f


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 -- camera.desktop $(DESTDIR)$(PREFIX)/share/applications/camera.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 -- gallery.desktop $(DESTDIR)$(PREFIX)/share/applications/gallery.desktop

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/camera.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/gallery.desktop

.PHONY: all clean distclean install uninstall
