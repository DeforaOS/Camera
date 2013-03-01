TARGETS	= camera.1 camera.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

camera.1: camera.xml
	./docbook.sh -P "$(PREFIX)" -- "camera.1"

camera.html: camera.xml
	./docbook.sh -P "$(PREFIX)" -- "camera.html"

clean:
	$(RM) -- $(camera.1_OBJS) $(camera.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "camera.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "camera.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "camera.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "camera.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "camera.html"

.PHONY: all clean distclean install uninstall
