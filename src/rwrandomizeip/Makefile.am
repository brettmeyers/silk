# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwrandomizeip

EXTRA_DIST = rwrandomizeip.pod
if HAVE_POD2MAN
man1_MANS = rwrandomizeip.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwrandomizeip_SOURCES = rwrandomizeip.c rwrandomizeip.h rwrand-shuffle.c



# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwrandomizeip-help.pl \
	tests/rwrandomizeip-version.pl \
	tests/rwrandomizeip-lone-command.pl \
	tests/rwrandomizeip-null-input.pl \
	tests/rwrandomizeip-no-dest.pl \
	tests/rwrandomizeip-empty-input.pl

# The following rely on random() which is not consistent across
# platforms
EXTRA_TESTS = \
	tests/rwrandomizeip-consistent.pl \
	tests/rwrandomizeip-load-save-table.pl \
	tests/rwrandomizeip-not-consistent.pl \
	tests/rwrandomizeip-stdin.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
