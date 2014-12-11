# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwswapbytes

EXTRA_DIST = rwswapbytes.pod
if HAVE_POD2MAN
man1_MANS = rwswapbytes.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwswapbytes_SOURCES = rwswapbytes.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwswapbytes-help.pl \
	tests/rwswapbytes-version.pl \
	tests/rwswapbytes-lone-command.pl \
	tests/rwswapbytes-no-switches.pl \
	tests/rwswapbytes-null-input.pl \
	tests/rwswapbytes-no-files.pl \
	tests/rwswapbytes-no-dest.pl \
	tests/rwswapbytes-big-empty.pl \
	tests/rwswapbytes-little-empty.pl \
	tests/rwswapbytes-swap-empty.pl \
	tests/rwswapbytes-big.pl \
	tests/rwswapbytes-little.pl \
	tests/rwswapbytes-swap-endian.pl \
	tests/rwswapbytes-stdin.pl \
	tests/rwswapbytes-big-v6.pl \
	tests/rwswapbytes-little-v6.pl
