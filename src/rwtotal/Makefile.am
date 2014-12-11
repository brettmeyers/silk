# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwtotal

EXTRA_DIST = rwtotal.pod
if HAVE_POD2MAN
man1_MANS = rwtotal.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwtotal_SOURCES = rwtotal.c rwtotal.h rwtotalsetup.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwtotal-help.pl \
	tests/rwtotal-version.pl \
	tests/rwtotal-lone-command.pl \
	tests/rwtotal-no-switches.pl \
	tests/rwtotal-no-files.pl \
	tests/rwtotal-null-input.pl \
	tests/rwtotal-key-conflict.pl \
	tests/rwtotal-sip-f-8.pl \
	tests/rwtotal-sip-f-16.pl \
	tests/rwtotal-sip-f-24.pl \
	tests/rwtotal-sip-l-8.pl \
	tests/rwtotal-sip-l-16.pl \
	tests/rwtotal-dip-f-8.pl \
	tests/rwtotal-dip-f-16.pl \
	tests/rwtotal-dip-f-24.pl \
	tests/rwtotal-dip-l-8.pl \
	tests/rwtotal-dip-l-16.pl \
	tests/rwtotal-dport.pl \
	tests/rwtotal-proto.pl \
	tests/rwtotal-packets.pl \
	tests/rwtotal-bytes.pl \
	tests/rwtotal-duration.pl \
	tests/rwtotal-sport-mn-rec.pl \
	tests/rwtotal-sport-mn-pkt.pl \
	tests/rwtotal-sport-mn-byt.pl \
	tests/rwtotal-sport-mx-rec.pl \
	tests/rwtotal-sport-mx-pkt.pl \
	tests/rwtotal-sport-mx-byt.pl \
	tests/rwtotal-summation.pl \
	tests/rwtotal-no-title.pl \
	tests/rwtotal-no-column.pl \
	tests/rwtotal-delimited.pl \
	tests/rwtotal-empty-input.pl \
	tests/rwtotal-multiple-inputs.pl \
	tests/rwtotal-multiple-inputs-v6.pl \
	tests/rwtotal-multiple-inputs-v4v6.pl \
	tests/rwtotal-copy-input.pl \
	tests/rwtotal-stdin.pl \
	tests/rwtotal-icmp-code.pl
