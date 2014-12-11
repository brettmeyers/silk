# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwappend

EXTRA_DIST = rwappend.pod
if HAVE_POD2MAN
man1_MANS = rwappend.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwappend_SOURCES = rwappend.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwappend-help.pl \
	tests/rwappend-version.pl \
	tests/rwappend-lone-command.pl \
	tests/rwappend-stdout.pl \
	tests/rwappend-null-output.pl \
	tests/rwappend-null-input.pl \
	tests/rwappend-one-file-big.pl \
	tests/rwappend-multiple-file-little.pl \
	tests/rwappend-create.pl \
	tests/rwappend-create-exists.pl \
	tests/rwappend-create-template.pl
