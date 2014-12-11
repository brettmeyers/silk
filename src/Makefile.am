# RCSIDENT($SiLK: Makefile.am 696fd479073b 2014-01-28 19:15:22Z mthomas $)

SILK_SRC_MAKE_BASE_LIBS = \
	 libsilk

SILK_SRC_MAKE_PACKING_LIBS = \
	 libflowsource

SILK_SRC_MAKE_RWFILTER = \
         rwfilter \
         plugins

SILK_SRC_MAKE_PYTHON_PLUGIN = \
         pysilk

SILK_SRC_MAKE_PACKING_TOOLS = \
         flowcap \
         rwflowpack \
         rwpollexec \
         sendrcv

SILK_SRC_MAKE_ANALYSIS_TOOLS = \
         num2dot \
         rwaddrcount \
         rwappend \
         rwbag \
         rwcat \
         rwcompare \
         rwconvert \
         rwcount \
         rwcut \
         rwfileinfo \
         rwgroup \
         rwmatch \
         rwnetmask \
         rwpmap \
         rwrandomizeip \
         rwrecgenerator \
         rwresolve \
         rwscan \
         rwset \
         rwsiteinfo \
         rwsort \
         rwsplit \
         rwstats \
         rwswapbytes \
         rwtotal \
         rwtuc \
         rwuniq

# Conditional analysis tools
SILK_SRC_MAKE_ANALYSIS_FIXBUF = rwipfix
SILK_SRC_MAKE_ANALYSIS_IPA = rwipa
SILK_SRC_MAKE_ANALYSIS_PCAP = rwptoflow
SILK_SRC_MAKE_ANALYSIS_PYTHONBIN = rwids

# Experimental code



SUBDIRS = $(SILK_SRC_MAKE_BASE_LIBS)

if BUILD_ANALYSIS_TOOLS


SUBDIRS += $(SILK_SRC_MAKE_RWFILTER)

if HAVE_PYTHON
SUBDIRS += $(SILK_SRC_MAKE_PYTHON_PLUGIN)
endif

SUBDIRS += $(SILK_SRC_MAKE_ANALYSIS_TOOLS)

if HAVE_PCAP
SUBDIRS += $(SILK_SRC_MAKE_ANALYSIS_PCAP)
endif
if HAVE_PYTHONBIN
SUBDIRS += $(SILK_SRC_MAKE_ANALYSIS_PYTHONBIN)
endif
if HAVE_IPA
SUBDIRS += $(SILK_SRC_MAKE_ANALYSIS_IPA)
endif

# Add the packing libraries here if either fixbuf is available or if
# the packing tools are being built.
if HAVE_FIXBUF
SUBDIRS += $(SILK_SRC_MAKE_PACKING_LIBS)
else
if BUILD_PACKING_TOOLS
SUBDIRS += $(SILK_SRC_MAKE_PACKING_LIBS)
endif
endif

if HAVE_FIXBUF
SUBDIRS += $(SILK_SRC_MAKE_ANALYSIS_FIXBUF)
endif

if BUILD_PACKING_TOOLS
SUBDIRS += $(SILK_SRC_MAKE_PACKING_TOOLS)
endif

else !BUILD_ANALYSIS_TOOLS


if BUILD_PACKING_TOOLS
SUBDIRS += \
	$(SILK_SRC_MAKE_PACKING_LIBS) \
	$(SILK_SRC_MAKE_PACKING_TOOLS)
endif

endif !BUILD_ANALYSIS_TOOLS

SUBDIRS += include/silk

DIST_SUBDIRS = \
	 $(SILK_SRC_MAKE_BASE_LIBS) \
	 $(SILK_SRC_MAKE_PACKING_LIBS) \
	 include/silk \
	 $(SILK_SRC_MAKE_RWFILTER) \
	 $(SILK_SRC_MAKE_PYTHON_PLUGIN) \
         $(SILK_SRC_MAKE_PACKING_TOOLS) \
	 $(SILK_SRC_MAKE_ANALYSIS_TOOLS) \
         $(SILK_SRC_MAKE_ANALYSIS_PCAP) \
         $(SILK_SRC_MAKE_ANALYSIS_IPA) \
	 $(SILK_SRC_MAKE_ANALYSIS_PYTHONBIN) \
	 $(SILK_SRC_MAKE_ANALYSIS_FIXBUF)
