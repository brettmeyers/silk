# RCSIDENT("$SiLK: Makefile.am 006bf6dc14e3 2013-01-22 20:56:29Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwipaexport rwipaimport

EXTRA_DIST = rwipaexport.pod rwipaimport.pod ipafilter.pod
if HAVE_POD2MAN
man1_MANS = rwipaexport.1 rwipaimport.1
man3_MANS = ipafilter.3
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(LIBIPA_CFLAGS) $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la $(LIBIPA_LDFLAGS)

if HAVE_STATIC_APPLICATIONS
ipafilter_libs = $(LDADD)
else
ipafilter_libs = 
endif

rwipaexport_SOURCES = rwipa.h rwipaexport.c rwipautils.c
rwipaexport_CFLAGS = $(AM_CFLAGS)

rwipaimport_SOURCES = rwipa.h rwipaimport.c rwipautils.c
rwipaimport_CFLAGS = $(AM_CFLAGS)


pkglib_LTLIBRARIES = ipafilter.la

ipafilter_la_SOURCES = ipafilter.c rwipautils.c
ipafilter_la_LDFLAGS = -module -avoid-version
ipafilter_la_LIBADD = $(ipafilter_libs) $(LIBIPA_LDFLAGS)


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwipaexport-help.pl \
	tests/rwipaexport-version.pl \
	tests/rwipaexport-lone-command.pl \
	tests/rwipaexport-no-catalog.pl \
	tests/rwipaimport-help.pl \
	tests/rwipaimport-version.pl \
	tests/rwipaimport-lone-command.pl \
	tests/rwipaimport-no-catalog.pl \
	tests/rwipaimport-no-endtime.pl \
	tests/rwipaimport-no-starttime.pl \
	tests/rwipaimport-no-config.pl \
	tests/rwipaexport-no-config.pl
