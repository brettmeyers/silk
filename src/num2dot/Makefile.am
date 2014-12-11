# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = num2dot

EXTRA_DIST = num2dot.pod
if HAVE_POD2MAN
man1_MANS = num2dot.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

num2dot_SOURCES = num2dot.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/num2dot-help.pl \
	tests/num2dot-version.pl \
	tests/num2dot-null-input.pl \
	tests/num2dot-rwcut-only.pl \
	tests/num2dot-ip-fields.pl \
	tests/num2dot-no-final-delim.pl
