# RCSIDENT("$SiLK: Makefile.am 965ed3e3a95d 2014-07-25 19:14:49Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwuniq

EXTRA_DIST = rwuniq.pod
if HAVE_POD2MAN
man1_MANS = rwuniq.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwuniq_SOURCES = rwuniq.c rwuniq.h rwuniqsetup.c

#include rwuniq-old.mk

# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwuniq-help.pl \
	tests/rwuniq-version.pl \
	tests/rwuniq-lone-command.pl \
	tests/rwuniq-no-switches.pl \
	tests/rwuniq-null-input.pl \
	tests/rwuniq-no-files.pl \
	tests/rwuniq-ports-proto-unix-sort.pl \
	tests/rwuniq-ports-proto-sorted.pl \
	tests/rwuniq-ports-proto-sorted-v6.pl \
	tests/rwuniq-ports-proto-presorted.pl \
	tests/rwuniq-ports-proto-multi-pre.pl \
	tests/rwuniq-sip-bytes.pl \
	tests/rwuniq-dip-bytes.pl \
	tests/rwuniq-dip-packets.pl \
	tests/rwuniq-sip-bytes-v6.pl \
	tests/rwuniq-dip-packets-v6.pl \
	tests/rwuniq-dport-all.pl \
	tests/rwuniq-stime-packets-flows.pl \
	tests/rwuniq-bin-time-stime.pl \
	tests/rwuniq-bin-time-etime.pl \
	tests/rwuniq-bin-time-stime-etime.pl \
	tests/rwuniq-bin-time-stime-etime-dur.pl \
	tests/rwuniq-elapsed-bytes.pl \
	tests/rwuniq-etime.pl \
	tests/rwuniq-stime-proto-sorted.pl \
	tests/rwuniq-collection-point.pl \
	tests/rwuniq-proto.pl \
	tests/rwuniq-sport-mn-rec.pl \
	tests/rwuniq-sport-mn-pkt.pl \
	tests/rwuniq-sport-mn-byt.pl \
	tests/rwuniq-sport-mx-rec.pl \
	tests/rwuniq-sport-mx-pkt.pl \
	tests/rwuniq-sport-mx-byt.pl \
	tests/rwuniq-sport-count-sip.pl \
	tests/rwuniq-sport-count-sip-v6.pl \
	tests/rwuniq-sport-count-sip-dip.pl \
	tests/rwuniq-sport-count-sip-pre.pl \
	tests/rwuniq-sport-count-sip-dip-pre.pl \
	tests/rwuniq-sport-count-sip-dip-v6.pl \
	tests/rwuniq-sport-count2-sip-dip-v6.pl \
	tests/rwuniq-sport-count-sip-pre-v6.pl \
	tests/rwuniq-no-title.pl \
	tests/rwuniq-no-column.pl \
	tests/rwuniq-delimited.pl \
	tests/rwuniq-legacytime-0.pl \
	tests/rwuniq-legacytime-1.pl \
	tests/rwuniq-empty-input.pl \
	tests/rwuniq-empty-input-presorted.pl \
	tests/rwuniq-empty-input-presorted-xargs.pl \
	tests/rwuniq-many-presorted.pl \
	tests/rwuniq-multiple-inputs.pl \
	tests/rwuniq-copy-input.pl \
	tests/rwuniq-stdin.pl \
	tests/rwuniq-icmpTypeCode.pl \
	tests/rwuniq-dport-icmp.pl \
	tests/rwuniq-icmp-dport.pl \
	tests/rwuniq-country-code-sip.pl \
	tests/rwuniq-country-code-dip.pl \
	tests/rwuniq-country-code-sip-v6.pl \
	tests/rwuniq-country-code-dip-v6.pl \
	tests/rwuniq-address-types-sip.pl \
	tests/rwuniq-address-types-dip.pl \
	tests/rwuniq-address-stype-proto-sorted.pl \
	tests/rwuniq-pmap-proto-port.pl \
	tests/rwuniq-pmap-src-service-host.pl \
	tests/rwuniq-pmap-dst-servhost.pl \
	tests/rwuniq-pmap-multiple.pl \
	tests/rwuniq-pmap-src-service-host-v6.pl \
	tests/rwuniq-pmap-dst-servhost-v6.pl \
	tests/rwuniq-pmap-multiple-v6.pl \
	tests/rwuniq-flowrate-payload.pl \
	tests/rwuniq-skplugin-test.pl \
	tests/rwuniq-pysilk-key.pl \
	tests/rwuniq-pysilk-value.pl \
	tests/rwuniq-pysilk-key-value.pl \
	tests/rwuniq-pysilk-simple-agg.pl

EXTRA_TESTS = \
	tests/rwuniq-flowrate-pps.pl \
	tests/rwuniq-flowrate-bps.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
