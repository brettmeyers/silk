#! /usr/bin/perl -w
# MD5: 2c69ae2e269c5667f5adf3119a82c89b
# TEST: ./rwfilter --scc=xa,xb,xc --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwfilter --scc=xa,xb,xc --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "2c69ae2e269c5667f5adf3119a82c89b";

check_md5_output($md5, $cmd);
