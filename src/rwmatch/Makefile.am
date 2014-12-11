# RCSIDENT("$SiLK: Makefile.am b0322a07081d 2014-09-24 18:55:26Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwmatch

EXTRA_DIST = rwmatch.pod
if HAVE_POD2MAN
man1_MANS = rwmatch.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwmatch_SOURCES = rwmatch.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwmatch-help.pl \
	tests/rwmatch-version.pl \
	tests/rwmatch-lone-command.pl \
	tests/rwmatch-too-few-args-1.pl \
	tests/rwmatch-too-few-args-2.pl \
	tests/rwmatch-empty-inputs.pl \
	tests/rwmatch-int-server.pl \
	tests/rwmatch-ext-server.pl \
	tests/rwmatch-int-server-v6.pl \
	tests/rwmatch-ext-server-v6.pl
