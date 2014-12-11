#! /usr/bin/perl -w
# MD5: bd40eefd186b769fec2ca8c9444102bb
# TEST: ./rwfilter --proto=17 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=17 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "bd40eefd186b769fec2ca8c9444102bb";

check_md5_output($md5, $cmd);
