# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwcompare

EXTRA_DIST = rwcompare.pod
if HAVE_POD2MAN
man1_MANS = rwcompare.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwcompare_SOURCES = rwcompare.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwcompare-help.pl \
	tests/rwcompare-version.pl \
	tests/rwcompare-lone-command.pl \
	tests/rwcompare-null-input.pl \
	tests/rwcompare-too-many-inputs.pl \
	tests/rwcompare-simple-fail.pl \
	tests/rwcompare-big.pl \
	tests/rwcompare-big-stdin.pl \
	tests/rwcompare-little-stdin.pl \
	tests/rwcompare-short-input.pl
