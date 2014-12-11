# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwaddrcount

EXTRA_DIST = rwaddrcount.pod
if HAVE_POD2MAN
man1_MANS = rwaddrcount.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwaddrcount_SOURCES = rwaddrcount.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwaddrcount-help.pl \
	tests/rwaddrcount-version.pl \
	tests/rwaddrcount-lone-command.pl \
	tests/rwaddrcount-no-switches.pl \
	tests/rwaddrcount-no-files.pl \
	tests/rwaddrcount-null-input.pl \
	tests/rwaddrcount-sip-stat.pl \
	tests/rwaddrcount-dip-stat.pl \
	tests/rwaddrcount-sip-rec.pl \
	tests/rwaddrcount-dip-rec.pl \
	tests/rwaddrcount-sip-ips.pl \
	tests/rwaddrcount-min-byte.pl \
	tests/rwaddrcount-max-byte.pl \
	tests/rwaddrcount-min-packet.pl \
	tests/rwaddrcount-max-packet.pl \
	tests/rwaddrcount-min-record.pl \
	tests/rwaddrcount-max-record.pl \
	tests/rwaddrcount-delimited.pl \
	tests/rwaddrcount-no-columns.pl \
	tests/rwaddrcount-column-sep.pl \
	tests/rwaddrcount-empty-input-rec.pl \
	tests/rwaddrcount-empty-input-stat.pl \
	tests/rwaddrcount-multiple-inputs.pl \
	tests/rwaddrcount-copy-input.pl \
	tests/rwaddrcount-stdin.pl \
	tests/rwaddrcount-sip-set.pl
