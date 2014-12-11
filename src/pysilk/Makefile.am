# RCSIDENT("$SiLK: Makefile.am 52b8a985f34b 2014-05-04 20:03:45Z mthomas $")

# Installed Targets

# Plugins (this is loaded by SiLK applications)
pkglib_LTLIBRARIES = silkpython.la

# Python extension (this is loaded by python binary or by silkpython)
pkgpython_LTLIBRARIES = pysilk.la pysilk_nl.la

pkgpython_PYTHON = python/__init__.py python/fglob.py \
		   python/plugin.py python/site.py 

# netsa-silk compatibility layer
pkgpython_PYTHON += python/_netsa_silk.py
python_PYTHON     = python/netsa_silk.py


# Manual pages
EXTRA_DIST = pysilk.pod silkpython.pod
if HAVE_POD2MAN
man3_MANS = pysilk.3 silkpython.3
endif

# Additional Targets

PYSILK_TEST = pysilk_test.py
PYSILK_TEST_SRC = python/$(PYSILK_TEST).in
EXTRA_DIST += $(PYSILK_TEST_SRC)
noinst_SCRIPTS = $(PYSILK_TEST)


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(PYTHON_CPPFLAGS) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS)

# The Python extension.  This extension gets loaded by a stand-alone
# Python invocation, so it must pull in the libsilk references.
pysilk_la_SOURCES     = pysilk.c pysilk_common.c pysilk_common.h
pysilk_la_LDFLAGS     = -module -avoid-version -shrext $(PYTHON_SO_EXTENSION) \
                        -rpath '$(pkgpythondir)' \
                        $(AM_LDFLAGS)
pysilk_la_LIBADD      = ../libsilk/libsilk.la

# A second version of the Python extension.  This is to be loaded by
# Python when it is loaded by a SiLK application, and it is not linked
# to libsilk; the SiLK app will provide those references.
pysilk_nl_la_SOURCES  = $(pysilk_la_SOURCES)
pysilk_nl_la_LDFLAGS  = $(pysilk_la_LDFLAGS)

# The SiLK plug-in that loads the above Python extension.  This is
# loaded by rwfilter, rwuniq, etc.
silkpython_la_SOURCES = silkpython.c pysilk_common.c pysilk_common.h
silkpython_la_LIBADD  =
silkpython_la_LDFLAGS = -module $(SILK_PLUGIN_LIBTOOL_FLAGS) \
                        $(PYTHON_LDFLAGS_EMBEDDED) \
                        $(AM_LDFLAGS)

MAKE_PYSILK_TEST = $(AM_V_GEN) \
  rm -f $@ $@.tmp ; \
  srcdir='' ; \
  test -f './$(PYSILK_TEST_SRC)' || srcdir='$(srcdir)/' ; \
  sed \
    -e 's|@PYTHON[@]|$(PYTHON)|g' \
    "$${srcdir}$(PYSILK_TEST_SRC)" >$@.tmp && \
  chmod +x $@.tmp && \
  mv $@.tmp $@

$(PYSILK_TEST): Makefile $(PYSILK_TEST_SRC)
	$(MAKE_PYSILK_TEST)

CLEANFILES = $(noinst_SCRIPTS)


# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


# "make check" in the tests directory will make this target with
# MINI_INST_DIR set to the location to install pysilk
#
# Note: we cannot install pysilk.la this way, since it must know the
# location of libsilk
pysilk_mini_install_dir = $(top_builddir)/tests/silk
pysilk_mini_install = $(pysilk_mini_install_dir)/pysilk-mini-install

pysilk-mini-install: $(pysilk_mini_install)

$(pysilk_mini_install): pysilk_nl.la $(pkgpython_PYTHON)
	test -d $(pysilk_mini_install_dir)
	@MINI_INST_DIR=`cd '$(pysilk_mini_install_dir)' && pwd` ; \
	  echo $(MAKE) do-pysilk-mini-install MINI_INST_DIR="$$MINI_INST_DIR" ; \
	  $(MAKE) do-pysilk-mini-install MINI_INST_DIR="$$MINI_INST_DIR"
	date > $@

do-pysilk-mini-install:
	test -n '$(MINI_INST_DIR)' && test -d '$(MINI_INST_DIR)'
	@list="$(pkgpython_PYTHON)" ; \
	for p in $$list ; do \
	    f=`basename $$p` ; \
	    test -f "$(MINI_INST_DIR)/$$f" && rm -f "$(MINI_INST_DIR)/$$f" ; \
	    d='' ; \
	    test -f "./$$p" || d='$(srcdir)/' ; \
	    echo cp "$$d$$p" "$(MINI_INST_DIR)/$$f" ; \
	    cp "$$d$$p" "$(MINI_INST_DIR)/$$f" ; \
	done
	@f=pysilk_nl.la ; \
	  echo " $(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=install $(INSTALL) $(INSTALL_STRIP_FLAG) $$f '$(MINI_INST_DIR)'" ; \
	  $(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=install $(INSTALL) $(INSTALL_STRIP_FLAG) $$f "$(MINI_INST_DIR)"

.PHONY: pysilk-mini-install do-pysilk-mini-install
