# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = mapsid rwsiteinfo

EXTRA_DIST = mapsid.pod rwsiteinfo.pod
if HAVE_POD2MAN
man1_MANS = mapsid.1 rwsiteinfo.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

mapsid_SOURCES = mapsid.c

rwsiteinfo_SOURCES = rwsiteinfo.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)
EXTRA_DIST += tests/rwsiteinfo-site.conf

TESTS = \
	tests/mapsid-bad-id.pl \
	tests/mapsid-by-name-or-id.pl \
	tests/mapsid-classes.pl \
	tests/mapsid-help.pl \
	tests/mapsid-lone-command.pl \
	tests/mapsid-no-such-id.pl \
	tests/mapsid-no-such-name.pl \
	tests/mapsid-version.pl \
	tests/rwsiteinfo-all-fields.pl \
	tests/rwsiteinfo-class-default-class.pl \
	tests/rwsiteinfo-class-default-type.pl \
	tests/rwsiteinfo-class-sensor.pl \
	tests/rwsiteinfo-class-types.pl \
	tests/rwsiteinfo-classes.pl \
	tests/rwsiteinfo-default-class-type.pl \
	tests/rwsiteinfo-default-class.pl \
	tests/rwsiteinfo-default-type.pl \
	tests/rwsiteinfo-delimited.pl \
	tests/rwsiteinfo-flowtypes.pl \
	tests/rwsiteinfo-help.pl \
	tests/rwsiteinfo-limit-classes.pl \
	tests/rwsiteinfo-limit-flowtypes.pl \
	tests/rwsiteinfo-limit-sensors.pl \
	tests/rwsiteinfo-limit-types.pl \
	tests/rwsiteinfo-lone-command.pl \
	tests/rwsiteinfo-no-title-no-final-no-columns.pl \
	tests/rwsiteinfo-raw-lists.pl \
	tests/rwsiteinfo-sensor-class.pl \
	tests/rwsiteinfo-sensors.pl \
	tests/rwsiteinfo-separators.pl \
	tests/rwsiteinfo-type-default-type.pl \
	tests/rwsiteinfo-types.pl \
	tests/rwsiteinfo-version.pl
