# RCSIDENT("$SiLK: Makefile.am f162742a8f51 2014-05-09 18:47:33Z mthomas $");

# The targets in this file are used only by "make check" (and support
# targest things like "make clean" and "make dist")

# tell "make dist" what to package
EXTRA_DIST = SiLKTests.pm daemon_test.py make-data.pl make-scandata.pl \
	 make-sendrcv-data.pl pysilk-plugin.py \
	 $(TEST_BAG_FILES_SOURCE) $(TEST_CC_FILES_SOURCE) \
	 $(TEST_SET_FILES_SOURCE) $(TEST_PMAP_FILES_SOURCE)

# targets that 'make check' makes
check_DATA = silk.conf made-test-data empty.rwf \
	 made-bag-files made-pmap-files made-set-files $(pysilk_mini_install)

# additional files that may be created by hand or by "make check" in
# particular directories
other_files = data.txt columnar-data.txt data-v6.txt columnar-data-v6.txt \
	 scandata.rwf small.pdu \
	 $(TEST_BAG_FILES) $(TEST_CC_FILES) \
	 $(TEST_PMAP_FILES) $(TEST_SET_FILES)


# commands
make_flows     = make-data.pl
make_scans     = make-scandata.pl
make_randfiles = make-sendrcv-data.pl


# the "silk.conf" file to use for testing
test_silk_conf = $(top_srcdir)/site/twoway/silk.conf

# get a local copy of silk.conf
silk.conf: $(test_silk_conf)
	-rm -f $@
	cp $(test_silk_conf) $@


# building SiLK flow files from text requires rwtuc
RWTUC = $(top_builddir)/src/rwtuc/rwtuc

$(RWTUC):
	cd $(top_builddir)/src/rwtuc && $(MAKE) rwtuc


# this is a data file to ensure header-only files are handled correctly
empty.rwf: $(RWTUC) silk.conf
	-rm -f $@
	$(RWTUC) --site-config-file=silk.conf --output-path=$@ < /dev/null

# the 'made-test-data' target will cause the following data file(s) to
# be created.  these are the files that applications use for testing
data_rwf = data.rwf
v6data_rwf = data-v6.rwf
data_files = $(data_rwf)
if SK_ENABLE_IPV6
data_files += $(v6data_rwf)
else
other_files += $(v6data_rwf)
endif

# allow 'make data.rwf' to work
$(data_files): made-test-data

# rules to make the data file(s)
if SK_ENABLE_IPV6

made-test-data: $(RWTUC) silk.conf $(make_flows)
	-rm -f $(data_files)
	srcdir='' ; \
	  test -f $(make_flows) || srcdir='$(srcdir)/' ; \
	  $(PERL) $${srcdir}$(make_flows) --status --ipv4-output='| $(RWTUC) --site-config-file=silk.conf --compression=best --output-path=$(data_rwf)' --ipv6-output='| $(RWTUC) --site-config-file=silk.conf --compression=best --output-path=$(v6data_rwf)' && date > $@

else

made-test-data: $(RWTUC) silk.conf $(make_flows)
	-rm -f $(data_files)
	srcdir='' ; \
	  test -f $(make_flows) || srcdir='$(srcdir)/' ; \
	  $(PERL) $${srcdir}$(make_flows) --status --ipv4-output='| $(RWTUC) --site-config-file=silk.conf --compression=best --output-path=$(data_rwf)' && date > $@

endif


# sets for testing rwset*
TEST_SET_FILES = set1-v4.set set2-v4.set set3-v4.set set4-v4.set
if SK_ENABLE_IPV6
if SK_ENABLE_SILK3_IPSETS
TEST_SET_FILES += set1-v6.set set2-v6.set set3-v6.set set4-v6.set
endif
endif
TEST_SET_FILES_SOURCE = set1-v4.txt set2-v4.txt set3-v4.txt set4-v4.txt \
	 set1-v6.txt set2-v6.txt set3-v6.txt set4-v6.txt

RWSETBUILD = $(top_builddir)/src/rwset/rwsetbuild

$(RWSETBUILD):
	cd $(top_builddir)/src/rwset && $(MAKE) rwsetbuild

$(TEST_SET_FILES): $(RWSETBUILD)

made-set-files: $(TEST_SET_FILES)
	date > $@
.txt.set:
	-rm -f $@ $@.tmp
	@srcdir='' ; \
	  test -f $< || srcdir='$(srcdir)/' ; \
	  echo "$(RWSETBUILD) $${srcdir}$< > $@.tmp" ; \
	  $(RWSETBUILD) $${srcdir}$< > $@.tmp
	mv $@.tmp $@


# bags for testing rwbag*
TEST_BAG_FILES = bag1-v4.bag bag2-v4.bag bag3-v4.bag
if SK_ENABLE_IPV6
TEST_BAG_FILES += bag1-v6.bag bag2-v6.bag bag3-v6.bag
endif
TEST_BAG_FILES_SOURCE = bag1-v4.txt bag2-v4.txt bag3-v4.txt \
	 bag1-v6.txt bag2-v6.txt bag3-v6.txt

RWBAGBUILD = $(top_builddir)/src/rwbag/rwbagbuild

$(RWBAGBUILD):
	cd $(top_builddir)/src/rwbag && $(MAKE) rwbagbuild

$(TEST_BAG_FILES): $(RWBAGBUILD)

made-bag-files: $(TEST_BAG_FILES)
	date > $@
.txt.bag:
	-rm -f $@ $@.tmp
	@srcdir='' ; \
	  test -f $< || srcdir='$(srcdir)/' ; \
	  echo "$(RWBAGBUILD) --bag-input=$${srcdir}$< > $@.tmp" ; \
	  $(RWBAGBUILD) --bag-input=$${srcdir}$< > $@.tmp
	mv $@.tmp $@


# scan data for testing rwscan
scandata.rwf: $(RWTUC) silk.conf $(make_scans)
	-rm -f $@
	srcdir='' ; \
	  test -f $(make_scans) || srcdir='$(srcdir)/' ; \
	  $(PERL) $${srcdir}$(make_scans) --status | $(RWTUC) --site-config-file=silk.conf --compression=best --output-path=$@

# for testing rwguess and rwflowpack
small.pdu: $(make_flows)
	-rm -f $@
	@srcdir='' ; \
	  test -f $(make_flows) || srcdir='$(srcdir)/' ; \
	  echo "$(PERL) $${srcdir}$(make_flows) --max=50000 --pdu-output=$@" ; \
	  $(PERL) $${srcdir}$(make_flows) --max=50000 --pdu-output=$@


# for testing the country-code plug-ins
TEST_CC_FILES = fake-cc.pmap
if SK_ENABLE_IPV6
TEST_CC_FILES += fake-cc-v6.pmap
endif
TEST_CC_FILES_SOURCE = fake-cc.csv fake-cc-v6.csv

RWGEOIP2CCMAP = $(top_builddir)/src/rwpmap/rwgeoip2ccmap

$(RWGEOIP2CCMAP):
	cd $(top_builddir)/src/rwpmap && $(MAKE) rwgeoip2ccmap

fake-cc.pmap: $(RWGEOIP2CCMAP) fake-cc.csv
	-rm -f $@ $@.tmp
	@f=fake-cc.csv  ; srcdir='' ; \
	  test -f $$f || srcdir='$(srcdir)/' ; \
	  echo "$(PERL) $(RWGEOIP2CCMAP) --csv < $${srcdir}$$f > $@.tmp" ; \
	  $(PERL) $(RWGEOIP2CCMAP) --csv < $${srcdir}$$f > $@.tmp
	mv $@.tmp $@

fake-cc-v6.pmap: $(RWGEOIP2CCMAP) fake-cc-v6.csv
	-rm -f $@ $@.tmp
	@f=fake-cc-v6.csv  ; srcdir='' ; \
	  test -f $$f || srcdir='$(srcdir)/' ; \
	  echo "$(PERL) $(RWGEOIP2CCMAP) --v6-csv < $${srcdir}$$f > $@.tmp" ; \
	  $(PERL) $(RWGEOIP2CCMAP) --v6-csv < $${srcdir}$$f > $@.tmp
	mv $@.tmp $@

# for testing the addrtype plug-in and rwpmap features
TEST_PMAP_FILES = address_types.pmap ip-map.pmap proto-port-map.pmap
if SK_ENABLE_IPV6
TEST_PMAP_FILES += ip-map-v6.pmap
endif
TEST_PMAP_FILES_SOURCE = address_types.txt ip-map.txt ip-map-v6.txt \
	 proto-port-map.txt

RWPMAPBUILD = $(top_builddir)/src/rwpmap/rwpmapbuild

$(RWPMAPBUILD):
	cd $(top_builddir)/src/rwpmap && $(MAKE) rwpmapbuild

$(TEST_PMAP_FILES): $(RWPMAPBUILD)

made-pmap-files: $(TEST_PMAP_FILES) $(TEST_CC_FILES)
	date > $@
.txt.pmap:
	-rm -f $@ $@.tmp
	@srcdir='' ; \
	  test -f $< || srcdir='$(srcdir)/' ; \
	  echo "$(RWPMAPBUILD) --input-file=$${srcdir}$< --output-file=$@.tmp" ; \
	  $(RWPMAPBUILD) --input-file=$${srcdir}$< --output-file=$@.tmp
	mv $@.tmp $@


# for generating files for testing rwsender/rwreceiver
made-sendrcv-data: $(make_randfiles)
	$(MAKE) remove-sendrcv-data
	@f=$(make_randfiles) ; srcdir='' ; \
	  test -f $$f || srcdir='$(srcdir)/' ; \
	  echo "$(PERL) $${srcdir}$$f --make-files --md5-out='$@'" ; \
	  $(PERL) $${srcdir}$$f --make-files --md5-out='$@'

remove-sendrcv-data:
	-@f=$(make_randfiles) ; srcdir='' ; \
	  test -f $$f || srcdir='$(srcdir)/' ; \
	  list=`$(PERL) $${srcdir}$$f --print-names` ; \
	  echo "rm -f made-sendrcv-data $${list}" ; \
	  rm -f made-sendrcv-data $${list}

# local mini-installation of PySiLK, but only build it if PySiLK
# support is enabled.
if HAVE_PYTHON
pysilk_mini_install = pysilk-mini-install
else
pysilk_mini_install =
endif

pysilk-mini-install:
	test -d silk || mkdir silk
	cd '$(top_builddir)/src/pysilk' && $(MAKE) $@

pysilk-mini-uninstall:
	-if test -d silk ; then \
	     rm -f silk/*.py silk/*.pyc silk/pysilk* ; \
	     rmdir silk ; \
	 else \
	     : ; \
	 fi


# other useful targets

# data in the form that "rwcut --fields=... --delimited" creates
data.txt: $(make_flows)
	-rm -f $@
	srcdir='' ; \
	  test -f $(make_flows) || srcdir='$(srcdir)/' ; \
	  $(PERL) $${srcdir}$(make_flows) --ipv4-output='$@'
data-v6.txt: $(make_flows)
	-rm -f $@
	srcdir='' ; \
	  test -f $(make_flows) || srcdir='$(srcdir)/' ; \
	  $(PERL) $${srcdir}$(make_flows) --ipv6-output='$@'

# in fixed-width columns (for IPv4, assumes --ipv6-policy=ignore)
columnar-data.txt: $(make_flows)
	-rm -f $@
	srcdir='' ; \
	  test -f $(make_flows) || srcdir='$(srcdir)/' ; \
	  $(PERL) $${srcdir}$(make_flows) --columnar --ipv4-output='$@'
columnar-data-v6.txt: $(make_flows)
	-rm -f $@
	srcdir='' ; \
	  test -f $(make_flows) || srcdir='$(srcdir)/' ; \
	  $(PERL) $${srcdir}$(make_flows) --columnar --ipv6-output='$@'


# have all outputs depend on this Makefile
$(check_DATA) $(data_files) $(other_files) made-bag-files made-pmap-files made-sendrcv-data made-set-files: Makefile

.PHONY: pysilk-mini-install pysilk-mini-uninstall remove-sendrcv-data

# clean up the files we create
CLEANFILES = $(check_DATA) $(data_files) $(other_files) \
	 config_vars.pyc daemon_test.pyc \
	 made-bag-files made-pmap-files made-set-files

clean-local: remove-sendrcv-data pysilk-mini-uninstall
