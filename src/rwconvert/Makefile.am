# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwallformats rwrtd2split

EXTRA_DIST =


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwallformats_SOURCES = rwallformats.c

rwrtd2split_SOURCES = rwrtd2split.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwallformats-help.pl \
	tests/rwallformats-version.pl \
	tests/rwallformats-lone-command.pl \
	tests/rwrtd2split-help.pl \
	tests/rwrtd2split-version.pl \
	tests/rwrtd2split-lone-command.pl

# Manually generated test
TESTS += \
	tests/rwallformats-all.pl
