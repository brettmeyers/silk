# RCSIDENT("$SiLK: Makefile.am e64c1fd2d834 2014-09-10 16:50:02Z mthomas $");

# Installed Targets

bin_PROGRAMS = rwset rwsetbuild rwsetcat rwsetmember rwsettool

EXTRA_DIST = rwset.pod rwsetbuild.pod rwsetcat.pod rwsetmember.pod \
	 rwsettool.pod
if HAVE_POD2MAN
man1_MANS = rwset.1 rwsetbuild.1 rwsetcat.1 rwsetmember.1 \
	 rwsettool.1
endif


# Build Rules

AM_CPPFLAGS = $(SK_SRC_INCLUDES) $(SK_CPPFLAGS)
AM_CFLAGS = $(WARN_CFLAGS) $(SK_CFLAGS)
AM_LDFLAGS = $(SK_LDFLAGS) $(STATIC_APPLICATIONS)
LDADD = ../libsilk/libsilk.la

rwset_SOURCES = rwset.c

rwsetbuild_SOURCES = rwsetbuild.c

rwsetcat_SOURCES = rwsetcat.c

rwsetmember_SOURCES = rwsetmember.c

rwsettool_SOURCES = rwsettool.c

# Global Rules
include $(top_srcdir)/build.mk
include $(top_srcdir)/lib-deps.mk


CLEANFILES = rwsetunion.1 rwsetintersect.1 rwsetunion rwsetintersect

# Tests

# Required files; variables defined in ../../build.mk
check_DATA = $(SILK_TESTSDIR) $(SILK_TESTDATA) $(SILK_TESTSETS)

EXTRA_DIST += $(TESTS)

TESTS = \
	tests/rwset-help.pl \
	tests/rwset-version.pl \
	tests/rwset-lone-command.pl \
	tests/rwsetbuild-help.pl \
	tests/rwsetbuild-version.pl \
	tests/rwsetbuild-lone-command.pl \
	tests/rwsetcat-help.pl \
	tests/rwsetcat-version.pl \
	tests/rwsetcat-lone-command.pl \
	tests/rwsettool-help.pl \
	tests/rwsettool-version.pl \
	tests/rwsettool-lone-command.pl \
	tests/rwsetmember-help.pl \
	tests/rwsetmember-version.pl \
	tests/rwsetmember-lone-command.pl \
	tests/rwset-no-switches.pl \
	tests/rwset-no-files.pl \
	tests/rwset-null-input.pl \
	tests/rwset-empty-input.pl \
	tests/rwset-sip.pl \
	tests/rwset-dip.pl \
	tests/rwset-nhip.pl \
	tests/rwset-sip-dip-1.pl \
	tests/rwset-sip-dip-2.pl \
	tests/rwset-multiple-inputs.pl \
	tests/rwset-stdin.pl \
	tests/rwset-copy-input.pl \
	tests/rwset-sip-v6.pl \
	tests/rwset-dip-v6.pl \
	tests/rwset-nhip-v6.pl \
	tests/rwset-sip-dip-1-v6.pl \
	tests/rwset-sip-dip-2-v6.pl \
	tests/rwsetcat-null-input.pl \
	tests/rwsetcat-count-ips.pl \
	tests/rwsetcat-cidr-blocks.pl \
	tests/rwsetcat-ip-ranges.pl \
	tests/rwsetcat-integer-ips.pl \
	tests/rwsetcat-hexadecimal-ips.pl \
	tests/rwsetcat-zero-pad-ips.pl \
	tests/rwsetcat-net-struct.pl \
	tests/rwsetcat-net-struct-ATS.pl \
	tests/rwsetcat-net-struct-12.pl \
	tests/rwsetcat-cidr-blocks-s1-v4.pl \
	tests/rwsetcat-cidr-blocks-s2-v4.pl \
	tests/rwsetcat-count-ips-s1-v4.pl \
	tests/rwsetcat-count-ips-s2-v4.pl \
	tests/rwsetcat-count-ips-filename-s1-v4.pl \
	tests/rwsetcat-count-ips-s1-s2-v4.pl \
	tests/rwsetcat-count-ips-filename-s1-s2-v4.pl \
	tests/rwsetcat-ip-ranges-s1-v4.pl \
	tests/rwsetcat-ip-ranges-filename-s1-v4.pl \
	tests/rwsetcat-ip-ranges-s2-v4.pl \
	tests/rwsetcat-net-struct-s1-v4.pl \
	tests/rwsetcat-net-struct-s2-v4.pl \
	tests/rwsetcat-net-struct-18-s1-v4.pl \
	tests/rwsetcat-net-struct-20-s2-v4.pl \
	tests/rwsetcat-net-struct-empty-v4.pl \
	tests/rwsetcat-net-struct-10-10-v4.pl \
	tests/rwsetcat-net-struct-10-7-v4.pl \
	tests/rwsetcat-net-struct-v6.pl \
	tests/rwsetcat-net-struct-48-v6.pl \
	tests/rwsetcat-net-struct-60.pl \
	tests/rwsetcat-cidr-blocks-s1-v6.pl \
	tests/rwsetcat-cidr-blocks-s2-v6.pl \
	tests/rwsetcat-count-ips-s1-v6.pl \
	tests/rwsetcat-count-ips-s2-v6.pl \
	tests/rwsetcat-ip-ranges-s1-v6.pl \
	tests/rwsetcat-ip-ranges-s2-v6.pl \
	tests/rwsetcat-net-struct-s1-v6.pl \
	tests/rwsetcat-net-struct-s2-v6.pl \
	tests/rwsetcat-net-struct-s1-58-v6.pl \
	tests/rwsetcat-net-struct-s2-60-v6.pl \
	tests/rwsetcat-net-struct-empty-v6.pl \
	tests/rwsetcat-net-struct-2001-db8-34-v6.pl \
	tests/rwsetcat-net-struct-2001-db8-31-v6.pl \
	tests/rwsettool-null-input.pl \
	tests/rwsettool-output-path.pl \
	tests/rwsettool-sample-none.pl \
	tests/rwsettool-union-s1-s2-v4.pl \
	tests/rwsettool-union-s2-s1-v4.pl \
	tests/rwsettool-intersect-s1-s2-v4.pl \
	tests/rwsettool-intersect-s2-s1-v4.pl \
	tests/rwsettool-difference-s1-s2-v4.pl \
	tests/rwsettool-difference-s2-s1-v4.pl \
	tests/rwsettool-symmet-diff-s1-s2-v4.pl \
	tests/rwsettool-symmet-diff-s2-s1-v4.pl \
	tests/rwsettool-union-s3-s4-v4.pl \
	tests/rwsettool-union-s4-s3-v4.pl \
	tests/rwsettool-intersect-s3-s4-v4.pl \
	tests/rwsettool-intersect-s4-s3-v4.pl \
	tests/rwsettool-difference-s3-s4-v4.pl \
	tests/rwsettool-difference-s4-s3-v4.pl \
	tests/rwsettool-symmet-diff-s3-s4-v4.pl \
	tests/rwsettool-symmet-diff-s4-s3-v4.pl \
	tests/rwsettool-mask-12-s1-v4.pl \
	tests/rwsettool-mask-12-s2-v4.pl \
	tests/rwsettool-mask-13-s1-v4.pl \
	tests/rwsettool-mask-13-s2-v4.pl \
	tests/rwsettool-mask-14-s1-v4.pl \
	tests/rwsettool-mask-14-s2-v4.pl \
	tests/rwsettool-mask-15-s1-v4.pl \
	tests/rwsettool-mask-15-s2-v4.pl \
	tests/rwsettool-mask-16-s1-v4.pl \
	tests/rwsettool-mask-16-s2-v4.pl \
	tests/rwsettool-mask-17-s1-v4.pl \
	tests/rwsettool-mask-17-s2-v4.pl \
	tests/rwsettool-mask-18-s1-v4.pl \
	tests/rwsettool-mask-18-s2-v4.pl \
	tests/rwsettool-mask-19-s1-v4.pl \
	tests/rwsettool-mask-19-s2-v4.pl \
	tests/rwsettool-mask-20-s1-v4.pl \
	tests/rwsettool-mask-20-s2-v4.pl \
	tests/rwsettool-mask-21-s1-v4.pl \
	tests/rwsettool-mask-21-s2-v4.pl \
	tests/rwsettool-mask-22-s1-v4.pl \
	tests/rwsettool-mask-22-s2-v4.pl \
	tests/rwsettool-mask-23-s1-v4.pl \
	tests/rwsettool-mask-23-s2-v4.pl \
	tests/rwsettool-mask-24-s1-v4.pl \
	tests/rwsettool-mask-24-s2-v4.pl \
	tests/rwsettool-mask-25-s1-v4.pl \
	tests/rwsettool-mask-25-s2-v4.pl \
	tests/rwsettool-mask-26-s1-v4.pl \
	tests/rwsettool-mask-26-s2-v4.pl \
	tests/rwsettool-mask-27-s1-v4.pl \
	tests/rwsettool-mask-27-s2-v4.pl \
	tests/rwsettool-mask-28-s1-v4.pl \
	tests/rwsettool-mask-28-s2-v4.pl \
	tests/rwsettool-mask-29-s1-v4.pl \
	tests/rwsettool-mask-29-s2-v4.pl \
	tests/rwsettool-mask-30-s1-v4.pl \
	tests/rwsettool-mask-30-s2-v4.pl \
	tests/rwsettool-union-s1-s2-v6.pl \
	tests/rwsettool-union-s2-s1-v6.pl \
	tests/rwsettool-intersect-s1-s2-v6.pl \
	tests/rwsettool-intersect-s2-s1-v6.pl \
	tests/rwsettool-difference-s1-s2-v6.pl \
	tests/rwsettool-difference-s2-s1-v6.pl \
	tests/rwsettool-symmet-diff-s1-s2-v6.pl \
	tests/rwsettool-symmet-diff-s2-s1-v6.pl \
	tests/rwsettool-union-s3-s4-v6.pl \
	tests/rwsettool-union-s4-s3-v6.pl \
	tests/rwsettool-intersect-s3-s4-v6.pl \
	tests/rwsettool-intersect-s4-s3-v6.pl \
	tests/rwsettool-difference-s3-s4-v6.pl \
	tests/rwsettool-difference-s4-s3-v6.pl \
	tests/rwsettool-symmet-diff-s3-s4-v6.pl \
	tests/rwsettool-symmet-diff-s4-s3-v6.pl \
	tests/rwsettool-mask-52-s1-v6.pl \
	tests/rwsettool-mask-52-s2-v6.pl \
	tests/rwsettool-mask-53-s1-v6.pl \
	tests/rwsettool-mask-53-s2-v6.pl \
	tests/rwsettool-mask-54-s1-v6.pl \
	tests/rwsettool-mask-54-s2-v6.pl \
	tests/rwsettool-mask-55-s1-v6.pl \
	tests/rwsettool-mask-55-s2-v6.pl \
	tests/rwsettool-mask-56-s1-v6.pl \
	tests/rwsettool-mask-56-s2-v6.pl \
	tests/rwsettool-mask-57-s1-v6.pl \
	tests/rwsettool-mask-57-s2-v6.pl \
	tests/rwsettool-mask-58-s1-v6.pl \
	tests/rwsettool-mask-58-s2-v6.pl \
	tests/rwsettool-mask-59-s1-v6.pl \
	tests/rwsettool-mask-59-s2-v6.pl \
	tests/rwsettool-mask-60-s1-v6.pl \
	tests/rwsettool-mask-60-s2-v6.pl \
	tests/rwsettool-mask-61-s1-v6.pl \
	tests/rwsettool-mask-61-s2-v6.pl \
	tests/rwsettool-mask-62-s1-v6.pl \
	tests/rwsettool-mask-62-s2-v6.pl \
	tests/rwsettool-mask-63-s1-v6.pl \
	tests/rwsettool-mask-63-s2-v6.pl \
	tests/rwsettool-mask-64-s1-v6.pl \
	tests/rwsettool-mask-64-s2-v6.pl \
	tests/rwsettool-mask-65-s1-v6.pl \
	tests/rwsettool-mask-65-s2-v6.pl \
	tests/rwsettool-mask-66-s1-v6.pl \
	tests/rwsettool-mask-66-s2-v6.pl \
	tests/rwsettool-mask-67-s1-v6.pl \
	tests/rwsettool-mask-67-s2-v6.pl \
	tests/rwsettool-mask-68-s1-v6.pl \
	tests/rwsettool-mask-68-s2-v6.pl \
	tests/rwsettool-mask-69-s1-v6.pl \
	tests/rwsettool-mask-69-s2-v6.pl \
	tests/rwsettool-mask-70-s1-v6.pl \
	tests/rwsettool-mask-70-s2-v6.pl \
	tests/rwsetbuild-null-input.pl \
	tests/rwsetbuild-ips.pl \
	tests/rwsetbuild-cidr.pl \
	tests/rwsetbuild-ranges.pl \
	tests/rwsetbuild-ips-v6.pl \
	tests/rwsetbuild-ips-s1-v4.pl \
	tests/rwsetbuild-ips-s2-v4.pl \
	tests/rwsetbuild-cidr-s1-v4.pl \
	tests/rwsetbuild-cidr-s2-v4.pl \
	tests/rwsetbuild-ranges-s1-v4.pl \
	tests/rwsetbuild-ranges-s2-v4.pl \
	tests/rwsetbuild-cidr-s1-v6.pl \
	tests/rwsetbuild-cidr-s2-v6.pl \
	tests/rwsetbuild-ranges-s1-v6.pl \
	tests/rwsetbuild-ranges-s2-v6.pl \
	tests/rwsetmember-null-input.pl \
	tests/rwsetmember-empy-input.pl \
	tests/rwsetmember-print-sets.pl \
	tests/rwsetmember-count.pl \
	tests/rwsetmember-print-sets-v6.pl \
	tests/rwsetmember-count-v6.pl \
	tests/rwsetmember-count-8-s1-s2-v4.pl \
	tests/rwsetmember-count-25-s1-s2-v4.pl \
	tests/rwsetmember-count-48-s1-s2-v6.pl \
	tests/rwsetmember-count-65-s1-s2-v6.pl

# above tests are automatically generated;
# those below are written by hand
TESTS += \
	tests/rwsettool-union-data-v4.pl \
	tests/rwsettool-union-data-v6.pl \
	tests/rwsettool-union-data-v4v6.pl \
	tests/rwsettool-sample-ratio.pl \
	tests/rwsettool-sample-size.pl \
	tests/rwsettool-sample-ratio-v6.pl \
	tests/rwsettool-sample-size-v6.pl
