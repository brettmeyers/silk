# RCSIDENT("$SiLK: Makefile.am 696fd479073b 2014-01-28 19:15:22Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwrecgenerator

EXTRA_DIST = rwrecgenerator.pod
if HAVE_POD2MAN
man1_MANS = rwrecgenerator.1
endif

EXTRA_PROGRAMS = rwrecgenerator-threaded skheap-rwrec-test


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwrecgenerator_SOURCES = rwrecgenerator.c \
	 skheap-rwrec.c skheap-rwrec.h \
	 stream-cache.c stream-cache.h

# The code in the threaded version is outdated.  It is also slower
# than the non-threaded version.
rwrecgenerator_threaded_SOURCES =  rwrecgenerator-threaded.c \
	 skheap-rwrec.c skheap-rwrec.h \
	 stream-cache.c stream-cache.h
rwrecgenerator_threaded_LDADD = ../libsilk/libsilk-thrd.la \
	 $(LDADD) $(PTHREAD_LDFLAGS)

skheap_rwrec_test_SOURCES = skheap-rwrec-test.c \
	 skheap-rwrec.c skheap-rwrec.h


# this example file can be used to build a prefix map to pass to
# rwrecgenerator's --sensor-prefix-map switch
EXTRA_DIST += sensor-map.txt

CLEANFILES = $(EXTRA_PROGRAMS)

# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwrecgenerator-help.pl \
	tests/rwrecgenerator-version.pl \
	tests/rwrecgenerator-lone-command.pl \
	tests/rwrecgenerator-null-input.pl \
	tests/rwrecgenerator-text-output.pl \
	tests/rwrecgenerator-binary-output.pl
