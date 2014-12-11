# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwfilter rwfglob

EXTRA_DIST = rwfilter.pod rwfglob.pod
if HAVE_POD2MAN
man1_MANS = rwfilter.1 rwfglob.1
endif


# Build Rules

MYCC = @CC@
rwfilter_extra =
cppflags_extra =
ldadd_common = ../libsilk/libsilk.la
ldadd_rwfilter = $(ldadd_common) $(PTHREAD_LDFLAGS)

CC = $(MYCC)
AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS) $(cppflags_extra)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)

rwfglob_SOURCES = fglob.c rwfglobapp.c
rwfglob_LDADD = $(ldadd_common)

rwfilter_SOURCES = fglob.c rwfilter.c rwfilter.h \
	 rwfiltercheck.c rwfiltersetup.c rwfilterthread.c rwfiltertuple.c \
	 $(rwfilter_extra)
rwfilter_LDADD = $(ldadd_rwfilter)


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwfilter-help.pl \
	tests/rwfilter-version.pl \
	tests/rwfilter-lone-command.pl \
	tests/rwfglob-help.pl \
	tests/rwfglob-version.pl \
	tests/rwfglob-lone-command.pl \
	tests/rwfglob-start-hour.pl \
	tests/rwfglob-start-end-hour.pl \
	tests/rwfglob-start-day.pl \
	tests/rwfglob-sensor-name.pl \
	tests/rwfglob-sensor-list.pl \
	tests/rwfglob-class.pl \
	tests/rwfglob-type.pl \
	tests/rwfglob-flowtype.pl \
	tests/rwfglob-type-sensor.pl \
	tests/rwfglob-bad-range.pl \
	tests/rwfilter-null-input.pl \
	tests/rwfilter-no-input.pl \
	tests/rwfilter-no-output.pl \
	tests/rwfilter-no-fltr-pass.pl \
	tests/rwfilter-no-filtr-fail.pl \
	tests/rwfilter-no-fltr-stats.pl \
	tests/rwfilter-fglob.pl \
	tests/rwfilter-sport.pl \
	tests/rwfilter-tuple-fwd.pl \
	tests/rwfilter-dp-25.pl \
	tests/rwfilter-tuple-rev.pl \
	tests/rwfilter-ap-25.pl \
	tests/rwfilter-tuple-both.pl \
	tests/rwfilter-sp-dp-25.pl \
	tests/rwfilter-empty.pl \
	tests/rwfilter-bytes.pl \
	tests/rwfilter-packets.pl \
	tests/rwfilter-bpp.pl \
	tests/rwfilter-duration.pl \
	tests/rwfilter-stime.pl \
	tests/rwfilter-etime.pl \
	tests/rwfilter-active-time.pl \
	tests/rwfilter-proto.pl \
	tests/rwfilter-print-stats-file.pl \
	tests/rwfilter-print-volume.pl \
	tests/rwfilter-print-volume-v6.pl \
	tests/rwfilter-scidr-fail.pl \
	tests/rwfilter-not-scidr-pass.pl \
	tests/rwfilter-saddr-fail.pl \
	tests/rwfilter-not-saddr-pass.pl \
	tests/rwfilter-sipset-fail.pl \
	tests/rwfilter-not-sipset-pass.pl \
	tests/rwfilter-any-cidr-fail.pl \
	tests/rwfilter-not-any-cidr-pass.pl \
	tests/rwfilter-any-addr-fail.pl \
	tests/rwfilter-not-any-addr-pass.pl \
	tests/rwfilter-anyset-fail.pl \
	tests/rwfilter-not-anyset-pass.pl \
	tests/rwfilter-max-pass.pl \
	tests/rwfilter-max-fail.pl \
	tests/rwfilter-max-pass-fail.pl \
	tests/rwfilter-type.pl \
	tests/rwfilter-icmp-type.pl \
	tests/rwfilter-icmp-code.pl \
	tests/rwfilter-flags-all.pl \
	tests/rwfilter-flags-init.pl \
	tests/rwfilter-flags-sess.pl \
	tests/rwfilter-attributes.pl \
	tests/rwfilter-application.pl \
	tests/rwfilter-ip-version.pl \
	tests/rwfilter-input-index.pl \
	tests/rwfilter-output-index.pl \
	tests/rwfilter-scidr-fail-v6.pl \
	tests/rwfilter-not-scidr-pass-v6.pl \
	tests/rwfilter-saddr-fail-v6.pl \
	tests/rwfilter-not-saddr-pass-v6.pl \
	tests/rwfilter-any-cidr-fail-v6.pl \
	tests/rwfilter-not-any-cidr-pass-v6.pl \
	tests/rwfilter-any-addr-fail-v6.pl \
	tests/rwfilter-not-any-addr-pass-v6.pl \
	tests/rwfilter-scc.pl \
	tests/rwfilter-dcc.pl \
	tests/rwfilter-scc-dcc.pl \
	tests/rwfilter-scc-v6.pl \
	tests/rwfilter-dcc-v6.pl \
	tests/rwfilter-scc-dcc-v6.pl \
	tests/rwfilter-stype-1.pl \
	tests/rwfilter-dtype-2.pl \
	tests/rwfilter-sport-dhcp.pl \
	tests/rwfilter-dport-http.pl \
	tests/rwfilter-sip-ntp.pl \
	tests/rwfilter-dip-internal.pl \
	tests/rwfilter-anyip-dhcp.pl \
	tests/rwfilter-two-pmaps.pl \
	tests/rwfilter-sip-ntp-v6.pl \
	tests/rwfilter-dip-internal-v6.pl \
	tests/rwfilter-anyip-dhcp-v6.pl \
	tests/rwfilter-two-pmaps-v6.pl \
	tests/rwfilter-flowrate-loaded-unused.pl \
	tests/rwfilter-payload-bytes.pl \
	tests/rwfilter-ipafilter-loaded-unused.pl \
	tests/rwfilter-python-loaded-unused.pl \
	tests/rwfilter-python-expr.pl \
	tests/rwfilter-python-file.pl \
	tests/rwfilter-multiple.pl \
	tests/rwfilter-stdin.pl \
	tests/rwfilter-xargs.pl \
	tests/rwfilter-threads.pl

EXTRA_TESTS = \
	tests/rwfilter-flowrate-bps.pl \
	tests/rwfilter-flowrate-pps.pl \
	tests/rwfilter-payload-rate.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
