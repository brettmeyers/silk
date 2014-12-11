# RCSIDENT("$SiLK: Makefile.am f162742a8f51 2014-05-09 18:47:33Z mthomas $")

# Installed Targets

if BUILD_PACKING_TOOLS
if !HAVE_PACKING_LOGIC
pkglib_LTLIBRARIES = packlogic-generic.la
endif

if HAVE_POD2MAN
man3_MANS = packlogic-generic.3
endif
endif

pkgdata_DATA = generic-silk.conf
EXTRA_DIST = packlogic-generic.pod silk.conf


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)

packlogic_generic_la_SOURCES = packlogic-generic.c
packlogic_generic_la_LDFLAGS = -module -avoid-version

generic-silk.conf: silk.conf
	$(AM_V_GEN)$(LN_S) $? $@

CLEANFILES = generic-silk.conf


# Global Rules
include $(top_srcdir)/build.mk
