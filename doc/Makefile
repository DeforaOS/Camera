TARGETS	= camera.1 camera.html gallery.1 gallery.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

camera.1: camera.xml
	./docbook.sh -P "$(PREFIX)" -- "camera.1"

camera.html: camera.css.xml camera.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "camera.html"

gallery.1: gallery.xml
	./docbook.sh -P "$(PREFIX)" -- "gallery.1"

gallery.html: gallery.css.xml gallery.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "gallery.html"

clean:
	$(RM) -- $(camera.1_OBJS) $(camera.html_OBJS) $(gallery.1_OBJS) $(gallery.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "camera.1"
	./docbook.sh -c -P "$(PREFIX)" -- "camera.html"
	./docbook.sh -c -P "$(PREFIX)" -- "gallery.1"
	./docbook.sh -c -P "$(PREFIX)" -- "gallery.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "camera.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "camera.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "gallery.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "gallery.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "camera.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "camera.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gallery.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gallery.html"

.PHONY: all clean distclean install uninstall
