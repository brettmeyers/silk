# RCSIDENT("$SiLK: Makefile.am f09105e13b0f 2014-05-23 20:14:04Z mthomas $")

# Installed Targets

bin_PROGRAMS = rwipfix2silk rwsilk2ipfix
bin_SCRIPTS = rwp2yaf2silk

EXTRA_DIST = rwipfix2silk.pod rwsilk2ipfix.pod rwp2yaf2silk.in
if HAVE_POD2MAN
if HAVE_PODSELECT
# Perl files have POD embedded in the file which podselect extracts
src2pod2man = rwp2yaf2silk.1
endif
man1_MANS = rwipfix2silk.1 rwsilk2ipfix.1 $(src2pod2man)
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(FIXBUF_CFLAGS) $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libflowsource/libflowsource.la \
	 ../libsilk/libsilk-thrd.la \
	 ../libsilk/libsilk.la \
	 $(FIXBUF_LDFLAGS) $(PTHREAD_LDFLAGS)

rwipfix2silk_SOURCES = rwipfix2silk.c

rwsilk2ipfix_SOURCES = rwsilk2ipfix.c

make_rwp2yaf2silk_edit = sed \
  -e 's|@PERL[@]|$(PERL)|g' \
  -e 's|@PACKAGE_STRING[@]|$(PACKAGE_STRING)|g' \
  -e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g' \
  -e 's|@SILK_VERSION_INTEGER[@]|$(SILK_VERSION_INTEGER)|g' \
  -e 's|@rwipfix2silk[@]|'$${my_rwipfix2silk}'|g'

MAKE_RWP2YAF2SILK = $(AM_V_GEN) \
  rm -f $@ $@.tmp ; \
  srcdir='' ; \
  test -f ./$@.in || srcdir=$(srcdir)/ ; \
  my_rwipfix2silk=`echo rwipfix2silk | sed -e '$(transform)'` ; \
  $(make_rwp2yaf2silk_edit) "$${srcdir}$@.in" >$@.tmp && \
  chmod +x $@.tmp && \
  mv $@.tmp $@

rwp2yaf2silk: Makefile rwp2yaf2silk.in
	$(MAKE_RWP2YAF2SILK)

rwp2yaf2silk.pod: rwp2yaf2silk.in
	$(AM_V_GEN)$(PODSELECT) $? > $@

MOSTLYCLEANFILES = rwp2yaf2silk.pod rwp2yaf2silk.tmp
CLEANFILES = rwp2yaf2silk

# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwipfix2silk-help.pl \
	tests/rwipfix2silk-version.pl \
	tests/rwipfix2silk-lone-command.pl \
	tests/rwsilk2ipfix-help.pl \
	tests/rwsilk2ipfix-version.pl \
	tests/rwsilk2ipfix-lone-command.pl \
	tests/rwp2yaf2silk-help.pl \
	tests/rwp2yaf2silk-version.pl \
	tests/rwp2yaf2silk-lone-command.pl \
	tests/rwsilk2ipfix-to-and-fro-data.pl \
	tests/rwsilk2ipfix-to-and-fro-data-v6.pl \
	tests/rwsilk2ipfix-to-and-fro-empty.pl \
	tests/rwsilk2ipfix-to-and-fro-multiple.pl \
	tests/rwsilk2ipfix-to-and-fro-stdin.pl \
	tests/rwsilk2ipfix-print-stat.pl \
	tests/rwsilk2ipfix-print-stat-v6.pl \
	tests/rwipfix2silk-print-stat.pl \
	tests/rwipfix2silk-print-stat-v6.pl
