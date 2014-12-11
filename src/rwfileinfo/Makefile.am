# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwfileinfo

EXTRA_DIST = rwfileinfo.pod
if HAVE_POD2MAN
man1_MANS = rwfileinfo.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwfileinfo_SOURCES = rwfileinfo.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwfileinfo-help.pl \
	tests/rwfileinfo-version.pl \
	tests/rwfileinfo-lone-command.pl \
	tests/rwfileinfo-recs-data.pl \
	tests/rwfileinfo-recs-empty.pl \
	tests/rwfileinfo-recs-stdin.pl \
	tests/rwfileinfo-vers-cmd-lines.pl \
	tests/rwfileinfo-length.pl \
	tests/rwfileinfo-byte-order.pl \
	tests/rwfileinfo-compression.pl
