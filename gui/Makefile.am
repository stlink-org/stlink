bin_PROGRAMS = stlink-gui

stlink_gui_SOURCES = \
	stlink-gui.c \
	stlink-gui.h

stlink_gui_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src \
	@GTK_CFLAGS@

stlink_gui_CFLAGS = \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	-DSTLINK_UI_DIR="\"$(uidir)\""

stlink_gui_LDFLAGS =  \
	$(AM_LDFLAGS)

stlink_gui_LDADD = \
	$(INTLLIBS) \
	$(top_builddir)/libstlink.a \
	@GTK_LIBS@

uidir = $(pkgdatadir)/ui
ui_DATA = stlink-gui.ui
EXTRA_DIST = $(ui_DATA)

-include $(top_srcdir)/git.mk
