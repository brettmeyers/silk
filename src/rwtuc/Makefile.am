# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwtuc

EXTRA_DIST = rwtuc.pod
if HAVE_POD2MAN
man1_MANS = rwtuc.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwtuc_SOURCES = rwtuc.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwtuc-help.pl \
	tests/rwtuc-version.pl \
	tests/rwtuc-lone-command.pl \
	tests/rwtuc-null-input.pl \
	tests/rwtuc-txt-and-back.pl \
	tests/rwtuc-txt-and-back-v6.pl
