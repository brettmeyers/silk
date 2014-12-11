#! /usr/bin/perl -w
# MD5: 2399bd843bfc10309f07963adf9d7a31
# TEST: ./rwrandomizeip --seed=38901 --consistent ../../tests/data.rwf - | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwrandomizeip = check_silk_app('rwrandomizeip');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwrandomizeip --seed=38901 --consistent $file{data} - | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "2399bd843bfc10309f07963adf9d7a31";

check_md5_output($md5, $cmd);
