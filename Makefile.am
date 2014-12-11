# RCSIDENT($SiLK: Makefile.am 24e5040f7590 2011-01-24 18:55:20Z mthomas $)

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = tests src site
DIST_SUBDIRS = doc $(SUBDIRS)

EXTRA_DIST = README.txt

# extra configuration files
EXTRA_DIST += $(PYTHON_INFO_PROG)

dist-hook: template-dir cvs-id-to-silk

template-dir:
	mkdir $(distdir)/src/template
	list="Makefile.am c-application.c c-file.c c-plugin.c header.h python.py" ; \
        for f in $$list ; do \
	    cp -p $(srcdir)/src/template/$$f $(distdir)/src/template/$$f ; \
	done

cvs-id-to-silk:
	perl -MFile::Find -0777 -we '$$File::Find::dont_use_nlink=1; for $$d (@ARGV){find(sub{$$n=$$File::Find::name;return if -l; if (/^(\.svn|CVS)$$/ && -d) {$$File::Find::prune = 1;return;} return unless -f _ && -w _ && -T; ($$a,$$m)=((stat(_))[8,9]); open(F,"+<$$_") or die "open $$n: $$!\n"; $$f=<F>; $$f=~s/\$$Id\:/\$$SiLK:/g; seek F,0,0 or die "seek $$n:$$!\n"; print F $$f; close F or die "$$n: $$!\n"; utime $$a,$$m,$$_ or die "utime $$n: $$!\n"},$$d);}' $(distdir)


install-data-hook: $(PYTHON_REMINDER) remind-silk-conf

remind-silk-conf:
	@echo ''
	@echo '************************************************************'
	@echo 'Example site configuration files have been installed in'
	@echo '    $(pkgdatadir)/*-silk.conf'
	@echo 'Choose the file that matches your packing logic plug-in'
	@echo 'and/or installation, rename it to silk.conf, customize it,'
	@echo 'and copy it to the root of the data directory:'
	@echo '    $(SILK_DATA_ROOTDIR)/silk.conf'
	@echo '************************************************************'



if HAVE_PYTHON
if BUILD_ANALYSIS_TOOLS
PYTHON_REMINDER = remind-python
endif
endif

PYSILK_INSTALL_TXT = INSTALL-PySiLK.txt

remind-python: $(PYSILK_INSTALL_TXT)
	-@if test "x$(PYTHON_SITE_PKG)" != "x$(PYTHON_DEFAULT_SITE_PKG)" ; then \
	cat $(PYSILK_INSTALL_TXT) ; \
	fi

$(PYSILK_INSTALL_TXT): Makefile
	-rm -f $@
	touch $@
	@echo '************************************************************' >> $@
	@echo "The PySiLK modules were installed outside Python's default" >> $@
	@echo "site directory.  To use SiLK in Python, set or modify your" >> $@
	@echo 'PYTHONPATH environment variable to include the directory:' >> $@
	@echo '    $(PYTHON_SITE_PKG)' >> $@
	@echo '************************************************************' >> $@


CLEANFILES = $(PYSILK_INSTALL_TXT)

DISTCLEANFILES = silk-$(PACKAGE_VERSION).spec $(SILK_SUMMARY_FILE)
