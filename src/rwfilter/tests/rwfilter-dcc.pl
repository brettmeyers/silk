#! /usr/bin/perl -w
# MD5: 886ede3cd5909f1786389edffc2eccb9
# TEST: ./rwfilter --dcc=xg,xj,xq --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwfilter --dcc=xg,xj,xq --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "886ede3cd5909f1786389edffc2eccb9";

check_md5_output($md5, $cmd);
