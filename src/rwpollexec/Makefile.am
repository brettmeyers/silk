# RCSIDENT("$SiLK: Makefile.am f162742a8f51 2014-05-09 18:47:33Z mthomas $")

# Installed Targets

sbin_PROGRAMS = rwpollexec

EXTRA_DIST = rwpollexec.pod
if HAVE_POD2MAN
man8_MANS = rwpollexec.8
endif

conf_files = rwpollexec.conf
EXTRA_DIST += rwpollexec.conf.in

init_d_scripts = rwpollexec.init.d
EXTRA_DIST += rwpollexec.init.d.in


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk-thrd.la ../libsilk/libsilk.la \
	$(PTHREAD_LDFLAGS)

rwpollexec_SOURCES = rwpollexec.c

rwpollexec.conf: Makefile rwpollexec.conf.in
	$(MAKE_CONF_FILE)
rwpollexec.init.d: Makefile rwpollexec.init.d.in
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
check_DATA = $(SILK_TESTSDIR)

EXTRA_DIST += $(TESTS)
EXTRA_DIST += tests/rwpollexec-daemon.py

MOSTLYCLEANFILES = tests/rwpollexec-daemon.pyc

TESTS = \
	tests/rwpollexec-help.pl \
	tests/rwpollexec-version.pl \
	tests/rwpollexec-lone-command.pl

# above tests are automatically generated;
# those below are written by hand
TESTS += \
	tests/rwpollexec-init-d.pl \
	tests/rwpollexec-zero.pl \
	tests/rwpollexec-nonzero.pl \
	tests/rwpollexec-killed.pl \
	tests/rwpollexec-sigterm.pl \
	tests/rwpollexec-sigkill.pl \
	tests/rwpollexec-set.pl \
	tests/rwpollexec-set-noarchive.pl \
	tests/rwpollexec-simul.pl
