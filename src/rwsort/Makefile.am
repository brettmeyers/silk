# RCSIDENT("$SiLK: Makefile.am e3bcfc2b6a7f 2014-09-24 20:04:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwcombine rwdedupe rwsort

EXTRA_DIST = rwcombine.pod rwdedupe.pod rwsort.pod
if HAVE_POD2MAN
man1_MANS = rwcombine.1 rwdedupe.1 rwsort.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwcombine_SOURCES = rwcombine.c rwcombine.h rwcombinesetup.c

rwdedupe_SOURCES = rwdedupe.c rwdedupe.h rwdedupesetup.c

rwsort_SOURCES = rwsort.c rwsort.h rwsortsetup.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwsort-help.pl \
	tests/rwsort-version.pl \
	tests/rwsort-lone-command.pl \
	tests/rwsort-no-switches.pl \
	tests/rwsort-no-files.pl \
	tests/rwsort-null-input.pl \
	tests/rwsort-sip.pl \
	tests/rwsort-dip.pl \
	tests/rwsort-proto-ports.pl \
	tests/rwsort-reverse-packets.pl \
	tests/rwsort-bytes.pl \
	tests/rwsort-stime.pl \
	tests/rwsort-elapsed.pl \
	tests/rwsort-class-type-sensor.pl \
	tests/rwsort-empty-input.pl \
	tests/rwsort-empty-input-presorted.pl \
	tests/rwsort-empty-input-presorted-xargs.pl \
	tests/rwsort-many-presorted.pl \
	tests/rwsort-multiple-inputs.pl \
	tests/rwsort-output-path.pl \
	tests/rwsort-stdin.pl \
	tests/rwsort-input-pipe.pl \
	tests/rwsort-sort-buffer.pl \
	tests/rwsort-presorted.pl \
	tests/rwsort-sip-v6.pl \
	tests/rwsort-dip-v6.pl \
	tests/rwsort-proto-ports-v6.pl \
	tests/rwsort-country-code-sip.pl \
	tests/rwsort-country-code-dip.pl \
	tests/rwsort-country-code-sip-v6.pl \
	tests/rwsort-country-code-dip-v6.pl \
	tests/rwsort-address-types-sip.pl \
	tests/rwsort-address-types-dip.pl \
	tests/rwsort-pmap-proto-port.pl \
	tests/rwsort-pmap-src-service-host.pl \
	tests/rwsort-pmap-dst-servhost.pl \
	tests/rwsort-pmap-multiple.pl \
	tests/rwsort-pmap-src-service-host-v6.pl \
	tests/rwsort-pmap-dst-servhost-v6.pl \
	tests/rwsort-pmap-multiple-v6.pl \
	tests/rwsort-flowrate-payload.pl \
	tests/rwsort-skplugin-test.pl \
	tests/rwsort-pysilk-key.pl \
	tests/rwsort-pysilk-simple-ipv4.pl \
	tests/rwsort-pysilk-simple-int.pl \
	tests/rwsort-pysilk-simple-enum.pl \
	tests/rwsort-pysilk-simple-ipv6.pl \
	tests/rwsort-rwuniq-nosort.pl \
	tests/rwsort-into-rwuniq.pl \
	tests/rwdedupe-help.pl \
	tests/rwdedupe-version.pl \
	tests/rwdedupe-lone-command.pl \
	tests/rwdedupe-no-switches.pl \
	tests/rwdedupe-no-files.pl \
	tests/rwdedupe-null-input.pl \
	tests/rwdedupe-empty-input.pl \
	tests/rwdedupe-one-copy.pl \
	tests/rwdedupe-two-copies.pl \
	tests/rwdedupe-buffer-size.pl \
	tests/rwdedupe-ignore-stime.pl \
	tests/rwdedupe-ignore-stime-v6.pl \
	tests/rwdedupe-one-copy-v6.pl \
	tests/rwdedupe-two-copies-v6.pl \
	tests/rwcombine-help.pl \
	tests/rwcombine-version.pl \
	tests/rwcombine-lone-command.pl \
	tests/rwcombine-no-switches.pl \
	tests/rwcombine-no-files.pl \
	tests/rwcombine-null-input.pl \
	tests/rwcombine-empty-input.pl \
	tests/rwcombine-empty-input-stats.pl \
	tests/rwcombine-standard-v4.pl \
	tests/rwcombine-standard-v4-stats.pl \
	tests/rwcombine-idle-time-v4.pl \
	tests/rwcombine-idle-time-v4-stats.pl \
	tests/rwcombine-buffer-idle-v4.pl \
	tests/rwcombine-buffer-idle-v4-stats.pl \
	tests/rwcombine-buffer-idle-v6.pl \
	tests/rwcombine-buffer-idle-v6-stats.pl

# above tests are automatically generated;
# those below are written by hand
TESTS += \
	tests/rwdedupe-mix-v4-v6.pl \
	tests/rwdedupe-mix-v4-v6-ignore.pl

EXTRA_TESTS = \
	tests/rwsort-flowrate-pps.pl \
	tests/rwsort-flowrate-bps.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
