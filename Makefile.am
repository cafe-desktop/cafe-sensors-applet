SUBDIRS = sensors-applet lib plugins pixmaps po help

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

appletdir =  $(datadir)/cafe-panel/applets
applet_in_files = org.cafe.applets.SensorsApplet.cafe-panel-applet.desktop.in
applet_DATA = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

uidir 		= $(datadir)/cafe-sensors-applet/ui
ui_DATA 	= SensorsApplet.xml

servicedir = $(datadir)/dbus-1/services
service_in_files = org.cafe.panel.applet.SensorsAppletFactory.service.in
service_DATA = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.SensorsAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
		-e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
		$< > $@

gsettingsschema_in_files = \
	org.cafe.sensors-applet.gschema.xml.in \
	org.cafe.sensors-applet.sensor.gschema.xml.in

gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

CLEANFILES = $(applet_DATA) \
	$(applet_in_files) \
	$(service_DATA) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-compile-warnings=no \
	CFLAGS='-Wno-deprecated-declarations'

EXTRA_DIST = $(ui_DATA) \
	$(applet_in_files).in \
	$(service_in_files) \
	$(gsettingsschema_in_files) \
	autogen.sh

DISTCLEANFILES	=

MSACLEANFILES = aclocal.m4 \
			compile \
			config.* \
			configure \
			depcomp \
			install-sh \
			ltmain.sh \
			missing \
			omf.make \
			xmldocs.make \
			INSTALL \
			$(top_srcdir)/sensors-applet/config.h*

msa-clean: maintainer-clean
	-test -z "$(MSACLEANFILES)" || rm -f $(MSACLEANFILES)
	-rm -rf m4;
	find . -name "Makefile.in" -exec rm \{\} \;

.PHONY: msa-clean

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
