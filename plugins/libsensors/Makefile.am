plugindir = $(libdir)/cafe-sensors-applet/plugins

# libsensors plugin
AM_CPPFLAGS = -DCAFELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""sensors-applet"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS) $(LIBSENSORS_CFLAGS)

plugin_LTLIBRARIES = liblibsensors.la

liblibsensors_la_SOURCES = \
	libsensors-plugin.h	\
	libsensors-plugin.c

liblibsensors_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) $(GLIB_LIBS) $(LIBSENSORS_LIBS)
liblibsensors_la_LIBADD = $(top_builddir)/lib/libcafe-sensors-applet-plugin.la

-include $(top_srcdir)/git.mk
