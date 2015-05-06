# RCSIDENT("$SiLK: Makefile.am a0de5ee69dff 2015-02-18 21:42:47Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwscan
if HAVE_PERL_DBI
have_dbi = rwscanquery
else
missing_dbi = rwscanquery
endif
bin_SCRIPTS = $(have_dbi)
noinst_SCRIPTS = $(missing_dbi)

EXTRA_DIST = rwscan.pod rwscanquery.in
if HAVE_POD2MAN
if HAVE_PODSELECT
if HAVE_PERL_DBI
# Perl files have POD embedded in the file which podselect extracts
src2pod2man = rwscanquery.1
endif
endif
man1_MANS = rwscan.1 $(src2pod2man)
endif

EXTRA_DIST += doc/db-mysql.sql doc/db-oracle.sql doc/db-postgres.sql


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la $(PTHREAD_LDFLAGS)

rwscan_SOURCES = rwscan.c rwscan.h rwscan_db.c rwscan_db.h \
	 rwscan_icmp.c rwscan_tcp.c rwscan_udp.c rwscan_utils.c \
	 rwscan_workqueue.c rwscan_workqueue.h

make_rwscanquery_edit = sed \
  -e 's|@PERL[@]|$(PERL)|g' \
  -e 's|@PACKAGE_STRING[@]|$(PACKAGE_STRING)|g' \
  -e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g' \
  -e 's|@SILK_VERSION_INTEGER[@]|$(SILK_VERSION_INTEGER)|g' \
  -e 's|@rwfilter[@]|'$${my_rwfilter}'|g' \
  -e 's|@rwset[@]|'$${my_rwset}'|g' \
  -e 's|@rwsetbuild[@]|'$${my_rwsetbuild}'|g' \
  -e 's|@rwsetcat[@]|'$${my_rwsetcat}'|g'

MAKE_RWSCANQUERY = $(AM_V_GEN) \
  rm -f $@ $@.tmp ; \
  srcdir='' ; \
  test -f ./$@.in || srcdir=$(srcdir)/ ; \
  my_rwfilter=`echo rwfilter | sed -e '$(transform)'` ; \
  my_rwset=`echo rwset | sed -e '$(transform)'` ; \
  my_rwsetbuild=`echo rwsetbuild | sed -e '$(transform)'` ; \
  my_rwsetcat=`echo rwsetcat | sed -e '$(transform)'` ; \
  $(make_rwscanquery_edit) "$${srcdir}$@.in" >$@.tmp && \
  chmod +x $@.tmp && \
  mv $@.tmp $@

rwscanquery: Makefile rwscanquery.in
	$(MAKE_RWSCANQUERY)

rwscanquery.pod : rwscanquery.in
	$(AM_V_GEN)$(PODSELECT) $? > $@

MOSTLYCLEANFILES = rwscanquery.pod rwscanquery.tmp
CLEANFILES = rwscanquery 

# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA) $(SILK_TESTSCAN)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwscan-help.pl \
	tests/rwscan-version.pl \
	tests/rwscan-lone-command.pl \
	tests/rwscan-null-input-blr.pl \
	tests/rwscan-missing-set-arg.pl \
	tests/rwscan-empty-input.pl \
	tests/rwscan-empty-input-blr.pl \
	tests/rwscan-hybrid.pl \
	tests/rwscan-trw-only.pl \
	tests/rwscan-blr-only.pl \
	tests/rwscanquery-help.pl \
	tests/rwscanquery-version.pl

# above tests are automatically generated;
# those below are written by hand
TESTS += \
	tests/rwscanquery-sqlite.pl
