# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwgroup

EXTRA_DIST = rwgroup.pod
if HAVE_POD2MAN
man1_MANS = rwgroup.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwgroup_SOURCES = rwgroup.c rwgroup.h rwgroupsetup.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwgroup-help.pl \
	tests/rwgroup-version.pl \
	tests/rwgroup-lone-command.pl \
	tests/rwgroup-null-input.pl \
	tests/rwgroup-too-many-args.pl \
	tests/rwgroup-no-delta-value.pl \
	tests/rwgroup-no-delta-field.pl \
	tests/rwgroup-empty-input.pl \
	tests/rwgroup-sport.pl \
	tests/rwgroup-sport-thresh.pl \
	tests/rwgroup-ipdelta.pl \
	tests/rwgroup-ports-delta.pl \
	tests/rwgroup-ips.pl \
	tests/rwgroup-ips-delta.pl \
	tests/rwgroup-ips-del-sum.pl \
	tests/rwgroup-ips-del-sum-thresh.pl \
	tests/rwgroup-ipdelta-v6.pl \
	tests/rwgroup-ips-v6.pl \
	tests/rwgroup-ips-del-sum-thresh-v6.pl \
	tests/rwgroup-country-code-sip.pl \
	tests/rwgroup-country-code-dip.pl \
	tests/rwgroup-address-types-sip.pl \
	tests/rwgroup-address-types-dip.pl \
	tests/rwgroup-pmap-proto-port.pl \
	tests/rwgroup-pmap-src-service-host.pl \
	tests/rwgroup-pmap-dst-servhost.pl \
	tests/rwgroup-pmap-multiple.pl \
	tests/rwgroup-pmap-src-service-host-v6.pl \
	tests/rwgroup-pmap-dst-servhost-v6.pl \
	tests/rwgroup-pmap-multiple-v6.pl \
	tests/rwgroup-flowrate-payload.pl \
	tests/rwgroup-pysilk-key.pl \
	tests/rwgroup-rwuniq-direct.pl \
	tests/rwgroup-into-rwuniq.pl

EXTRA_TESTS = \
	tests/rwgroup-flowrate-pps.pl \
	tests/rwgroup-flowrate-bps.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
