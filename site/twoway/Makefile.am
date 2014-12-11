# RCSIDENT("$SiLK: Makefile.am f162742a8f51 2014-05-09 18:47:33Z mthomas $")

# Installed Targets

if BUILD_PACKING_TOOLS
if !HAVE_PACKING_LOGIC
pkglib_LTLIBRARIES = packlogic-twoway.la
endif

if HAVE_POD2MAN
man3_MANS = packlogic-twoway.3
endif
endif

pkgdata_DATA = twoway-silk.conf
EXTRA_DIST = packlogic-twoway.pod silk.conf


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)

packlogic_twoway_la_SOURCES = packlogic-twoway.c
packlogic_twoway_la_LDFLAGS = -module -avoid-version

twoway-silk.conf: silk.conf
	$(AM_V_GEN)$(LN_S) $? $@

CLEANFILES = twoway-silk.conf


# Global Rules
include $(top_srcdir)/build.mk
