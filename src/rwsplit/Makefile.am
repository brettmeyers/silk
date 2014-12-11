# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwsplit

EXTRA_DIST = rwsplit.pod
if HAVE_POD2MAN
man1_MANS = rwsplit.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwsplit_SOURCES = rwsplit.c


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwsplit-help.pl \
	tests/rwsplit-version.pl \
	tests/rwsplit-lone-command.pl \
	tests/rwsplit-missing-limit.pl \
	tests/rwsplit-missing-basename.pl \
	tests/rwsplit-multiple-limit.pl \
	tests/rwsplit-null-input.pl \
	tests/rwsplit-empty-input.pl

# Manually generated test
TESTS += \
	tests/rwsplit-flow-10k.pl \
	tests/rwsplit-ip-5k.pl \
	tests/rwsplit-pkt-10M.pl \
	tests/rwsplit-byte-10M-max.pl

# The following rely on random() which is not consistent across
# platforms
EXTRA_TESTS = \
	tests/rwsplit-byte-10M-sample.pl \
	tests/rwsplit-byte-10M-file.pl \
	tests/rwsplit-pkt-50k-sample-file.pl
EXTRA_DIST += $(EXTRA_TESTS)

if RUN_EXTRA_CHECKS
TESTS += $(EXTRA_TESTS)
endif
