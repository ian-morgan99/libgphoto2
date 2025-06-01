AM_CPPFLAGS = $(INCLUDES)
AM_CFLAGS = -Wall -Wmissing-prototypes

EXTRA_LTLIBRARIES =
CAMLIBS =

if ENABLE_PENTAXMODERN_GLIB
CAMLIBS += libgphoto2_cam_pentaxmodern.la
libgphoto2_cam_pentaxmodern_la_SOURCES = pentaxmodern.c pentaxmodern.h
libgphoto2_cam_pentaxmodern_la_LDFLAGS = -module -avoid-version $(NO_UNDEFINED)
libgphoto2_cam_pentaxmodern_la_CFLAGS = $(AM_CFLAGS) $(PENTAXMODERN_GLIB_CFLAGS)
libgphoto2_cam_pentaxmodern_la_LIBADD = $(PENTAXMODERN_GLIB_LIBS) $(LIBGPHOTO2_LIBS) $(LIBGPHOTO2_LDFLAGS)
else
EXTRA_LTLIBRARIES += libgphoto2_cam_pentaxmodern.la
endif
