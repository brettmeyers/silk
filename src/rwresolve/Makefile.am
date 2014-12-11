# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwresolve

EXTRA_DIST = rwresolve.pod
if HAVE_POD2MAN
man1_MANS = rwresolve.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS) $(ADNS_CFLAGS) $(CARES_CFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la $(ADNS_LDFLAGS) $(CARES_LDFLAGS)

rwresolve_SOURCES = rwresolve.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwresolve-help.pl \
	tests/rwresolve-version.pl \
	tests/rwresolve-null-input.pl \
	tests/rwresolve-default-fields.pl \
	tests/rwresolve-no-final-delim.pl \
	tests/rwresolve-one-field.pl \
	tests/rwresolve-all-too-large.pl \
	tests/rwresolve-one-too-large.pl \
	tests/rwresolve-delimiter.pl
