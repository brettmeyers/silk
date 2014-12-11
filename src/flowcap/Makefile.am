# RCSIDENT("$SiLK: Makefile.am f162742a8f51 2014-05-09 18:47:33Z mthomas $");

# Installed Targets

sbin_PROGRAMS = flowcap

EXTRA_DIST = flowcap.pod
if HAVE_POD2MAN
man8_MANS = flowcap.8
endif

conf_files = flowcap.conf
EXTRA_DIST += flowcap.conf.in

init_d_scripts = flowcap.init.d
EXTRA_DIST += flowcap.init.d.in


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(FIXBUF_CFLAGS) $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)

flowcap_SOURCES = flowcap.c flowcap.h flowcapsetup.c
flowcap_LDADD = ../libflowsource/libflowsource.la \
	 ../libsilk/libsilk-thrd.la \
	 ../libsilk/libsilk.la \
	 $(FIXBUF_LDFLAGS) $(PTHREAD_LDFLAGS)


flowcap.conf: Makefile flowcap.conf.in
	$(MAKE_CONF_FILE)
flowcap.init.d: Makefile flowcap.init.d.in
	$(MAKE_INIT_D_SCRIPT)

all-local: $(conf_files) $(init_d_scripts)
install-data-local: install-conf-files install-init-d-scripts
uninstall-local: uninstall-conf-files uninstall-init-d-scripts

CLEANFILES = $(conf_files) $(init_d_scripts)

# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA)

EXTRA_DIST += $(TESTS)
EXTRA_DIST += tests/flowcap-daemon.py

MOSTLYCLEANFILES = tests/flowcap-daemon.pyc

TESTS = \
	tests/flowcap-help.pl \
	tests/flowcap-version.pl \
	tests/flowcap-lone-command.pl

# above tests are automatically generated;
# those below are written by hand
TESTS += \
	tests/flowcap-init-d.pl \
	tests/flowcap-netflowv5-v4.pl \
	tests/flowcap-netflowv5-any-v4.pl \
	tests/flowcap-netflowv5-v6.pl \
	tests/flowcap-ipfix-v4.pl \
	tests/flowcap-ipfix-any-v4.pl \
	tests/flowcap-ipfix-v6.pl \
	tests/flowcap-ipfixv6-v6.pl
