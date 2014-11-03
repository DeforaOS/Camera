TARGETS	= $(OBJDIR)camera.1 $(OBJDIR)camera.html $(OBJDIR)gallery.1 $(OBJDIR)gallery.html
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)camera.1: camera.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)camera.1"

$(OBJDIR)camera.html: camera.css.xml camera.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)camera.html"

$(OBJDIR)gallery.1: gallery.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)gallery.1"

$(OBJDIR)gallery.html: gallery.css.xml gallery.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)gallery.html"

clean:
	$(RM) -- $(camera.1_OBJS) $(camera.html_OBJS) $(gallery.1_OBJS) $(gallery.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "camera.1"
	./docbook.sh -c -P "$(PREFIX)" -- "camera.html"
	./docbook.sh -c -P "$(PREFIX)" -- "gallery.1"
	./docbook.sh -c -P "$(PREFIX)" -- "gallery.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)camera.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)camera.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)gallery.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)gallery.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "camera.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "camera.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gallery.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gallery.html"

.PHONY: all clean distclean install uninstall
