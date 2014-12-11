#! /usr/bin/perl -w
# MD5: 7783cc02e88e976576c11e6c37a5072e
# TEST: ./rwrandomizeip --seed=38901 ../../tests/data.rwf stdout | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwrandomizeip = check_silk_app('rwrandomizeip');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwrandomizeip --seed=38901 $file{data} stdout | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "7783cc02e88e976576c11e6c37a5072e";

check_md5_output($md5, $cmd);
