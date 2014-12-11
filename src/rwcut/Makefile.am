# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwcut

EXTRA_DIST = rwcut.pod
if HAVE_POD2MAN
man1_MANS = rwcut.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwcut_SOURCES = rwcut.c rwcut.h rwcutsetup.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwcut-help.pl \
	tests/rwcut-version.pl \
	tests/rwcut-lone-command.pl \
	tests/rwcut-null-input.pl \
	tests/rwcut-sip-v4.pl \
	tests/rwcut-dip-v4.pl \
	tests/rwcut-5tuple-v4.pl \
	tests/rwcut-5tuple-v4-as-v6.pl \
	tests/rwcut-5tuple-v6.pl \
	tests/rwcut-bytes-pkt.pl \
	tests/rwcut-flag-fields.pl \
	tests/rwcut-stime.pl \
	tests/rwcut-time-fields.pl \
	tests/rwcut-site-fields.pl \
	tests/rwcut-misc-fields.pl \
	tests/rwcut-default-fields.pl \
	tests/rwcut-all-fields.pl \
	tests/rwcut-all-fields-v6.pl \
	tests/rwcut-dry-run.pl \
	tests/rwcut-rec-count1.pl \
	tests/rwcut-rec-count2.pl \
	tests/rwcut-rec-count3.pl \
	tests/rwcut-rec-count4.pl \
	tests/rwcut-rec-count5.pl \
	tests/rwcut-rec-count6.pl \
	tests/rwcut-rec-count7.pl \
	tests/rwcut-rec-count8.pl \
	tests/rwcut-rec-count-err1.pl \
	tests/rwcut-rec-count-err2.pl \
	tests/rwcut-rec-count-err3.pl \
	tests/rwcut-rec-count-err4.pl \
	tests/rwcut-rec-count-err5.pl \
	tests/rwcut-no-title.pl \
	tests/rwcut-no-final-del.pl \
	tests/rwcut-no-columns.pl \
	tests/rwcut-column-sep.pl \
	tests/rwcut-legacy-0.pl \
	tests/rwcut-legacy-1.pl \
	tests/rwcut-empty-input.pl \
	tests/rwcut-multiple-inputs.pl \
	tests/rwcut-multiple-inputs-v6.pl \
	tests/rwcut-copy-input.pl \
	tests/rwcut-stdin.pl \
	tests/rwcut-icmpTypeCode.pl \
	tests/rwcut-icmp-type.pl \
	tests/rwcut-icmpTypeCode-v6.pl \
	tests/rwcut-icmp-type-v6.pl \
	tests/rwcut-country-code.pl \
	tests/rwcut-country-code-v6.pl \
	tests/rwcut-address-types.pl \
	tests/rwcut-pmap-proto-port.pl \
	tests/rwcut-pmap-src-service-host.pl \
	tests/rwcut-pmap-dst-servhost.pl \
	tests/rwcut-pmap-multiple.pl \
	tests/rwcut-pmap-src-service-host-v6.pl \
	tests/rwcut-pmap-dst-servhost-v6.pl \
	tests/rwcut-pmap-multiple-v6.pl \
	tests/rwcut-skplugin-test.pl \
	tests/rwcut-pysilk-lower-port.pl \
	tests/rwcut-pysilk-lower-port2.pl \
	tests/rwcut-pysilk-private-ipv6.pl \
	tests/rwcut-pysilk-simple-reg.pl \
	tests/rwcut-pysilk-country.pl

EXTRA_TESTS = \
	tests/rwcut-flowrate.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
