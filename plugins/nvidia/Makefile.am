# NVIDIA plugin
plugindir = $(libdir)/cafe-sensors-applet/plugins

AM_CPPFLAGS = -DCAFELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""sensors-applet"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS) $(NVIDIA_CFLAGS)

plugin_LTLIBRARIES = libnvidia.la

libnvidia_la_SOURCES = \
	nvidia-plugin.h	\
	nvidia-plugin.c

libnvidia_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) $(GLIB_LIBS) $(NVIDIA_LIBS)
libnvidia_la_LIBADD = $(top_builddir)/lib/libcafe-sensors-applet-plugin.la

-include $(top_srcdir)/git.mk
