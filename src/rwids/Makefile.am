# RCSIDENT("$SiLK: Makefile.am f09105e13b0f 2014-05-23 20:14:04Z mthomas $")

# Installed Targets

if HAVE_PYTHONBIN
bin_SCRIPTS = rwidsquery
endif

EXTRA_DIST = rwidsquery.pod
if HAVE_POD2MAN
if HAVE_PYTHONBIN
man1_MANS = rwidsquery.1
endif
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)

EXTRA_DIST += rwidsquery.in rwidsquery24.in

if HAVE_PYTHON24
rwidsquery_tmpl = rwidsquery24.in
else
rwidsquery_tmpl = rwidsquery.in
endif

make_rwidsquery_edit = sed \
  -e 's|@PYTHON[@]|$(PYTHON)|g' \
  -e 's|@PACKAGE_STRING[@]|$(PACKAGE_STRING)|g' \
  -e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g' \
  -e 's|@rwfilter[@]|'$${my_rwfilter}'|g'

MAKE_RWIDSQUERY = $(AM_V_GEN) \
  rm -f $@ $@.tmp ; \
  srcdir='' ; \
  test -f ./$(rwidsquery_tmpl) || srcdir=$(srcdir)/ ; \
  my_rwfilter=`echo rwfilter | sed -e '$(transform)'` ; \
  $(make_rwidsquery_edit) "$${srcdir}$(rwidsquery_tmpl)" >$@.tmp && \
  chmod +x $@.tmp && \
  mv $@.tmp $@

rwidsquery: Makefile $(rwidsquery_tmpl)
	$(MAKE_RWIDSQUERY)

MOSTLYCLEANFILES = rwidsquery.tmp
CLEANFILES = rwidsquery

# Global Rules
include $(top_srcdir)/build.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwidsquery-help.pl \
	tests/rwidsquery-version.pl \
	tests/rwidsquery-lone-command.pl \
	tests/rwidsquery-no-input.pl \
	tests/rwidsquery-fast.pl \
	tests/rwidsquery-full.pl \
	tests/rwidsquery-rule.pl \
	tests/rwidsquery-rule-no-date.pl
