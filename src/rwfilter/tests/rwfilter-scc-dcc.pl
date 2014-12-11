#! /usr/bin/perl -w
# MD5: bd40eefd186b769fec2ca8c9444102bb
# TEST: ./rwfilter --scc=xz --dcc=xz --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwfilter --scc=xz --dcc=xz --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "bd40eefd186b769fec2ca8c9444102bb";

check_md5_output($md5, $cmd);
