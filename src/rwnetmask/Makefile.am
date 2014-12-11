# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwnetmask

EXTRA_DIST = rwnetmask.pod
if HAVE_POD2MAN
man1_MANS = rwnetmask.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwnetmask_SOURCES = rwnetmask.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwnetmask-help.pl \
	tests/rwnetmask-version.pl \
	tests/rwnetmask-lone-command.pl \
	tests/rwnetmask-null-input.pl \
	tests/rwnetmask-no-files.pl \
	tests/rwnetmask-no-switches.pl \
	tests/rwnetmask-sip-24.pl \
	tests/rwnetmask-sip-24-dip-16.pl \
	tests/rwnetmask-empty-input.pl \
	tests/rwnetmask-stdin.pl \
	tests/rwnetmask-sip-120-v6.pl \
	tests/rwnetmask-sip-120-dip-64-v6.pl
