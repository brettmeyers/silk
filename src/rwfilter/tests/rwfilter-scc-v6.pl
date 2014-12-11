#! /usr/bin/perl -w
# MD5: 045e70fdd12830a478f15fa47b17e663
# TEST: ./rwfilter --scc=xa,xb,xc --pass=stdout ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
check_features(qw(ipv6));
my $cmd = "$rwfilter --scc=xa,xb,xc --pass=stdout $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "045e70fdd12830a478f15fa47b17e663";

check_md5_output($md5, $cmd);
