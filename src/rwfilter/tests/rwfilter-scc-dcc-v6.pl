#! /usr/bin/perl -w
# MD5: 17cda13997907b0693feb84c9fcf0829
# TEST: ./rwfilter --scc=xz --dcc=xz --pass=stdout ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
check_features(qw(ipv6));
my $cmd = "$rwfilter --scc=xz --dcc=xz --pass=stdout $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "17cda13997907b0693feb84c9fcf0829";

check_md5_output($md5, $cmd);
