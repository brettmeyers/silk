# RCSIDENT("$Id$")

#######################################################################
#
# This instructional block should be removed from a Makefile.am once
# it is copied into the source tree.
#
# For documentation on the Makefile.am syntax, see the automake info
# page.
#
# The layout we are using roughly follows these rules:
#
# 1. Try to keep things in alphabetical order, both variable names and
# the values within each variable.
#
# 2. Have the RCSIDENT() tags at the top of the file.
#
# 3. Next specify the "Installed Targets" by creating a variable
# <path>_<type> and setting its value to the targets of <type> that
# will be installed at <path>.  For example, bin_PROGRAMS specifies
# 'programs' to be installed in the 'bin' directory that the user sets
# when she runs configure, sbin_SCRIPTS specify scripts to be
# installed into the 'sbin' directory.
#
# The common variables are given below; remove any that you don't use.
# For any manual pages that get installed, use <file>.N, where N is a
# digit; a rule will convert the .pod file to .N.
#
# 4. Next, specify the "Additional Targets".  These are programs or
# libraries that are built but not installed.  Use the same <type>s as
# from the previous rule, but use "noinst" as the location to say that
# it is not installed.
#
# 5. Next are the "Build Rules".  Any libraries or linking flags that
# are common for all targets in this directory can be specified in the
# LDADD.  If the library is build with libtool, specify a '.la'
# extention, e.g., "libflowsource.la".  For other SiLK libraries,
# given the path relative this directory; for example, you would use
# "../libsilk/libsilk.la" to link with libsilk.  For all the targets
# that were listed above, you need to specify a <target>_SOURCES
# variable to list the source files, except change any
# non-alphanumeric character in <target> to an underscore.  If the
# target requires special values for CFLAGS, LDFLAGS, or LDADD, add
# them.
#
# 6. At the bottom we include the $(top_srcdir)/build.mk file that
# contains the Global Rules that automake will add to each Makefile.in
# that it generates.
#
####################################################################


# Installed Targets

bin_PROGRAMS = # user applications

bin_SCRIPTS = # user scripts

lib_LIBRARIES = # normal libraries

lib_LTLIBRARIES = # libtool libraries

EXTRA_DIST = # include the man pages here with a .pod extension
if HAVE_POD2MAN
man1_MANS = # various man pages; use the extention '.1', '.3', etc
man3_MANS = 
man5_MANS = 
man8_MANS = 
endif


pkglib_LIBRARIES = # libraries for lib/silk

pkglib_LTLIBRARIES = # libtool libraries for lib/silk (and plug-ins)

sbin_PROGRAMS = # administrator programs

sbin_SCRIPTS = # administrator scripts

dist_sysconfig_DATA = # config files for /etc


# Additional Targets

noinst_PROGRAMS = # similiar to above


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)   # flags for this directory
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)           # flags for this directory
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS) # flags for this directory
LDADD = ../libsilk/libsilk.la                     # libs for this directory

bar_SOURCES = # sources to make 'bar'
bar_CFLAGS = # special CFLAGS that 'bar' requires; if used add $(AM_CFLAGS)

foo_SOURCES = # sources to make 'foo'
foo_LDADD = # special libraries for 'foo'; LDADD above not used

libbaz_a_SOURCES = # sources to make 'libbaz.a'

libqux_la_SOURCES = # sources to make 'libqux.la'


# Global Rules
include $(top_srcdir)/build.mk
