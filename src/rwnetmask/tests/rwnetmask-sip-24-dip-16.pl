#! /usr/bin/perl -w
# MD5: b75467d0a2d7adb0a30fbbbedeaa6e2b
# TEST: ./rwnetmask --dip-prefix=16 --sip-prefix=24 ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwnetmask --dip-prefix=16 --sip-prefix=24 $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "b75467d0a2d7adb0a30fbbbedeaa6e2b";

check_md5_output($md5, $cmd);
