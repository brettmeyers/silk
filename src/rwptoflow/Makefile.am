# RCSIDENT("$SiLK: Makefile.am f162742a8f51 2014-05-09 18:47:33Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwpdedupe rwpmatch rwptoflow
if HAVE_PYTHONBIN
bin_SCRIPTS = rwpcut
endif

noinst_LTLIBRARIES = rwp2f_minbytes.la

EXTRA_DIST = rwptoflow.pod rwpmatch.pod rwpdedupe.pod rwpcut.pod
if HAVE_POD2MAN
man1_MANS = rwptoflow.1 rwpmatch.1 rwpdedupe.1
if HAVE_PYTHONBIN
man1_MANS += rwpcut.1
endif
endif


# Additional Targets

EXTRA_PROGRAMS = rwpgenoffsetdata
# $(EXTRA_PROGRAMS) only need to appear in one of bin_PROGRAMS,
# noinst_PROGRAMS, or check_PROGRAMS
#check_PROGRAMS = $(EXTRA_PROGRAMS)
# Swap the comment character between bin_PROGRAMS and noinst_PROGRAMS
# to determine whether to install the test programs in $(bindir)
#bin_PROGRAMS += $(EXTRA_PROGRAMS)
noinst_PROGRAMS = $(EXTRA_PROGRAMS)


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la $(PCAP_LDFLAGS)

EXTRA_DIST += rwpcut.in rwpcut24.in

if HAVE_PYTHON24
rwpcut_tmpl = rwpcut24.in

MAKE_RWPCUT24 = $(AM_V_GEN) \
  rm -f $@ $@.tmp ; \
  srcdir='' ; \
  test -f ./$(rwpcut_tmpl) || srcdir=$(srcdir)/ ; \
  $(make_python_script_edit) $${srcdir}$(rwpcut_tmpl) >$@.tmp && \
  chmod +x $@.tmp && \
  mv $@.tmp $@

rwpcut: Makefile $(rwpcut_tmpl)
	$(MAKE_RWPCUT24)

else

rwpcut: Makefile rwpcut.in
	$(MAKE_PYTHON_SCRIPT)

endif

MOSTLYCLEANFILES = rwpcut.tmp
CLEANFILES = rwpcut

rwp2f_minbytes_la_SOURCES = rwp2f_minbytes.c rwppacketheaders.h
rwp2f_minbytes_la_LDFLAGS = -module $(SILK_PLUGIN_LIBTOOL_FLAGS)

rwpdedupe_SOURCES = rwpdedupe.c rwppacketheaders.h

rwpgenoffsetdata_SOURCES = rwpgenoffsetdata.c rwppacketheaders.h

rwpmatch_SOURCES = rwpmatch.c rwppacketheaders.h

rwptoflow_SOURCES = rwptoflow.c rwppacketheaders.h
rwptoflow_LDADD = $(LDADD)


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk
