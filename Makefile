PACKAGE	= Camera
VERSION	= 0.0.0
SUBDIRS	= data po src tools
RM	= rm -f
LN	= ln -f
TAR	= tar -czvf


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE)) || exit; done

clean:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) clean) || exit; done

distclean:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) distclean) || exit; done

dist:
	$(RM) -r -- $(PACKAGE)-$(VERSION)
	$(LN) -s -- . $(PACKAGE)-$(VERSION)
	@$(TAR) $(PACKAGE)-$(VERSION).tar.gz -- \
		$(PACKAGE)-$(VERSION)/data/Makefile \
		$(PACKAGE)-$(VERSION)/data/camera.desktop \
		$(PACKAGE)-$(VERSION)/data/gallery.desktop \
		$(PACKAGE)-$(VERSION)/data/project.conf \
		$(PACKAGE)-$(VERSION)/po/Makefile \
		$(PACKAGE)-$(VERSION)/po/gettext.sh \
		$(PACKAGE)-$(VERSION)/po/POTFILES \
		$(PACKAGE)-$(VERSION)/po/fr.po \
		$(PACKAGE)-$(VERSION)/po/project.conf \
		$(PACKAGE)-$(VERSION)/src/camera.c \
		$(PACKAGE)-$(VERSION)/src/main.c \
		$(PACKAGE)-$(VERSION)/src/Makefile \
		$(PACKAGE)-$(VERSION)/src/camera.h \
		$(PACKAGE)-$(VERSION)/src/project.conf \
		$(PACKAGE)-$(VERSION)/tools/gallery.c \
		$(PACKAGE)-$(VERSION)/tools/Makefile \
		$(PACKAGE)-$(VERSION)/tools/project.conf \
		$(PACKAGE)-$(VERSION)/Makefile \
		$(PACKAGE)-$(VERSION)/config.h \
		$(PACKAGE)-$(VERSION)/project.conf
	$(RM) -- $(PACKAGE)-$(VERSION)

install:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) install) || exit; done

uninstall:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) uninstall) || exit; done

.PHONY: all subdirs clean distclean dist install uninstall
