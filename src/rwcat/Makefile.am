# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwcat

EXTRA_DIST = rwcat.pod
if HAVE_POD2MAN
man1_MANS = rwcat.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwcat_SOURCES = rwcat.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwcat-help.pl \
	tests/rwcat-version.pl \
	tests/rwcat-lone-command.pl \
	tests/rwcat-null-input.pl \
	tests/rwcat-no-cat.pl \
	tests/rwcat-one-file.pl \
	tests/rwcat-multiple-files.pl \
	tests/rwcat-stdin.pl \
	tests/rwcat-xargs.pl \
	tests/rwcat-xargs-stdin.pl \
	tests/rwcat-big-endian.pl \
	tests/rwcat-little-endian.pl \
	tests/rwcat-note-add.pl \
	tests/rwcat-note-file.pl \
	tests/rwcat-one-file-v6.pl \
	tests/rwcat-big-endian-v6.pl \
	tests/rwcat-little-endian-v6.pl
