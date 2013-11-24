PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-camera.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-camera.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-gallery.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-gallery.desktop

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-camera.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-gallery.desktop

.PHONY: all clean distclean install uninstall
