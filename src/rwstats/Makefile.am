# RCSIDENT("$SiLK: Makefile.am 965ed3e3a95d 2014-07-25 19:14:49Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwstats

EXTRA_DIST = rwstats.pod
if HAVE_POD2MAN
man1_MANS = rwstats.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwstats_SOURCES = rwstats.c rwstats.h rwstatssetup.c \
	interval.h intervalstats.c rwstatslegacy.c rwstatsproto.c

#include rwstats-old.mk

# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwstats-help.pl \
	tests/rwstats-legacy-help.pl \
	tests/rwstats-version.pl \
	tests/rwstats-lone-command.pl \
	tests/rwstats-no-switches.pl \
	tests/rwstats-no-files.pl \
	tests/rwstats-null-input.pl \
	tests/rwstats-sip-top-flo-p4.pl \
	tests/rwstats-dip-top-flo-p4.pl \
	tests/rwstats-sip-top-byt-c100.pl \
	tests/rwstats-dip-top-pkt-t25000.pl \
	tests/rwstats-sdip-top-byt-c2.pl \
	tests/rwstats-sip24-top-pkt-p2.pl \
	tests/rwstats-sip24-top-pkt-p1.pl \
	tests/rwstats-dip16-bot-byt-c10.pl \
	tests/rwstats-sip-top-flo-p4-v6.pl \
	tests/rwstats-dip-top-pkt-t25000-v6.pl \
	tests/rwstats-sdip-top-byt-c2-v6.pl \
	tests/rwstats-sport-top-flo-p5.pl \
	tests/rwstats-dport-top-flo-t8000.pl \
	tests/rwstats-sdport-top-flo-c5.pl \
	tests/rwstats-sport-sip-pkt-byt-c10.pl \
	tests/rwstats-proto-top-pkt-c15.pl \
	tests/rwstats-proto-bot-pkt-c15.pl \
	tests/rwstats-sport-sip-dist-t5000.pl \
	tests/rwstats-dport-dip-dist-t5000.pl \
	tests/rwstats-proto-sdip-dist-c5.pl \
	tests/rwstats-proto-sdip-dist-c5-v6.pl \
	tests/rwstats-bin-time-stime.pl \
	tests/rwstats-bin-time-etime.pl \
	tests/rwstats-bin-time-stime-etime.pl \
	tests/rwstats-bin-time-stime-etime-dur.pl \
	tests/rwstats-integer-ips.pl \
	tests/rwstats-no-titles.pl \
	tests/rwstats-no-columns.pl \
	tests/rwstats-column-sep.pl \
	tests/rwstats-delimited.pl \
	tests/rwstats-proto-stats.pl \
	tests/rwstats-overall-stats.pl \
	tests/rwstats-empty-input.pl \
	tests/rwstats-empty-input-presorted.pl \
	tests/rwstats-empty-input-presorted-xargs.pl \
	tests/rwstats-many-presorted.pl \
	tests/rwstats-multiple-inputs.pl \
	tests/rwstats-multiple-inputs-v6.pl \
	tests/rwstats-multiple-inputs-v4v6.pl \
	tests/rwstats-copy-input.pl \
	tests/rwstats-stdin.pl \
	tests/rwstats-icmp-top-byt-p5.pl \
	tests/rwstats-dport-icmp-top-p16.pl \
	tests/rwstats-icmp-dport-top-p16.pl \
	tests/rwstats-sport-bot-flo-c4.pl \
	tests/rwstats-dport-bot-byt-c4.pl \
	tests/rwstats-multi-inputs-3-5.pl \
	tests/rwstats-multi-inputs-3-5-pre.pl \
	tests/rwstats-country-code-sip.pl \
	tests/rwstats-country-code-dip.pl \
	tests/rwstats-country-code-sip-v6.pl \
	tests/rwstats-country-code-dip-v6.pl \
	tests/rwstats-address-types-sip.pl \
	tests/rwstats-address-types-dip.pl \
	tests/rwstats-pmap-proto-port.pl \
	tests/rwstats-pmap-src-service-host.pl \
	tests/rwstats-pmap-dst-servhost.pl \
	tests/rwstats-pmap-multiple.pl \
	tests/rwstats-pmap-src-service-host-v6.pl \
	tests/rwstats-pmap-dst-servhost-v6.pl \
	tests/rwstats-pmap-multiple-v6.pl \
	tests/rwstats-flowrate-payload.pl \
	tests/rwstats-skplugin-test.pl \
	tests/rwstats-pysilk-key.pl \
	tests/rwstats-pysilk-value.pl \
	tests/rwstats-pysilk-key-value.pl \
	tests/rwstats-pysilk-simple-agg.pl

EXTRA_TESTS = \
	tests/rwstats-flowrate-pps.pl \
	tests/rwstats-flowrate-bps.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
