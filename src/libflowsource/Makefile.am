# RCSIDENT("$SiLK: Makefile.am 521bac0ed616 2014-09-25 18:47:54Z mthomas $");

# Installed Targets

lib_LTLIBRARIES = libflowsource.la
libflowsource_version = 9:0:1
# At previous release: libflowsource_version = 9:0:1

EXTRA_DIST = sensor.conf.pod
if HAVE_POD2MAN
man5_MANS = sensor.conf.5
endif

pkginclude_HEADERS = libflowsource.h probeconf.h \
	 $(extra_headers1) $(extra_headers2) $(extra_headers3) \
	 $(extra_headers4) $(extra_headers5) $(extra_headers6)


# Dependencies on libfixbuf
if HAVE_FIXBUF
extra_check_programs = check-struct
extra_sources2 = skipfix.c ipfixsource.c
extra_headers2 = skipfix.h
endif


# Additional Targets

EXTRA_PROGRAMS = circbuf-test $(extra_check_programs)
# $(EXTRA_PROGRAMS) only need to appear in one of bin_PROGRAMS,
# noinst_PROGRAMS, or check_PROGRAMS
#check_PROGRAMS = $(EXTRA_PROGRAMS)
# Swap the comment character between bin_PROGRAMS and noinst_PROGRAMS
# to determine whether to install the test programs in $(bindir)
#bin_PROGRAMS += $(EXTRA_PROGRAMS)
noinst_PROGRAMS = $(EXTRA_PROGRAMS)


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(FIXBUF_CFLAGS) $(WARN_CFLAGS) $(SK_CFLAGS)
LDADD =	 ../libsilk/libsilk-thrd.la \
	 ../libsilk/libsilk.la \
	 $(FIXBUF_LDFLAGS) $(PTHREAD_LDFLAGS)

libflowsource_la_SOURCES = circbuf.c circbuf.h \
	 pdusource.c udpsource.c udpsource.h probeconf.c \
	 probeconfparse.y probeconfscan.h probeconfscan.l v5pdu.h \
	 $(extra_sources1) $(extra_sources2) $(extra_sources3) \
	 $(extra_sources4) $(extra_sources5) $(extra_sources6)
libflowsource_la_LDFLAGS = -version-info $(libflowsource_version)
#libflowsource_la_LIBADD  = $(LDADD)
extra_sources1 = 

check_struct_SOURCES = check-struct.c
check_struct_LDADD = libflowsource.la $(LDADD)

circbuf_test_SOURCES = circbuf-test.c
circbuf_test_LDADD = libflowsource.la $(LDADD)

# add switches to flex that remove unused functions
AM_LFLAGS = $(FLEX_NOFUNS)

AM_YFLAGS = -d
probeconfscan.$(OBJEXT) probeconfscan.lo: probeconfparse.h

CLEANFILES = $(EXTRA_PROGRAMS)

# Global Rules
include $(top_srcdir)/build.mk
