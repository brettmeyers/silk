# RCSIDENT("$SiLK: Makefile.am 3497cd65cfca 2013-10-28 16:24:32Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwcount

EXTRA_DIST = rwcount.pod
if HAVE_POD2MAN
man1_MANS = rwcount.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwcount_SOURCES = rwcount.c rwcount.h rwcountsetup.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwcount-help.pl \
	tests/rwcount-version.pl \
	tests/rwcount-lone-command.pl \
	tests/rwcount-null-input.pl \
	tests/rwcount-no-switches.pl \
	tests/rwcount-b1-l1.pl \
	tests/rwcount-b30-l2.pl \
	tests/rwcount-b900-l3.pl \
	tests/rwcount-b3600.pl \
	tests/rwcount-b86400-l1.pl \
	tests/rwcount-b3600-l2.pl \
	tests/rwcount-start-epoch.pl \
	tests/rwcount-start-epoch-skip-0.pl \
	tests/rwcount-end-epoch.pl \
	tests/rwcount-start-epoch-week.pl \
	tests/rwcount-start-end-epoch.pl \
	tests/rwcount-milli-bin-start.pl \
	tests/rwcount-milli-bin.pl \
	tests/rwcount-reversed-input.pl \
	tests/rwcount-delimited.pl \
	tests/rwcount-no-columns.pl \
	tests/rwcount-column-sep.pl \
	tests/rwcount-legacy-0.pl \
	tests/rwcount-legacy-1.pl \
	tests/rwcount-empty-input.pl \
	tests/rwcount-empty-input-b3600.pl \
	tests/rwcount-multiple-inputs.pl \
	tests/rwcount-multiple-inputs-v6.pl \
	tests/rwcount-multiple-inputs-v4v6.pl \
	tests/rwcount-copy-input.pl \
	tests/rwcount-stdin.pl \
	tests/rwcount-b1800-l3.pl \
	tests/rwcount-b1800-l4.pl \
	tests/rwcount-b1800-l5.pl \
	tests/rwcount-b1800-l6.pl
