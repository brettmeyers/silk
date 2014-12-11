#! /usr/bin/perl -w
# MD5: ebe316ba466c889ed61f531fac957731
# TEST: echo 10.4.0.0/14 | ../rwset/rwsetbuild | ./rwbagtool --intersect=- ../../tests/bag2-v4.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v4bag2} = get_data_or_exit77('v4bag2');
my $cmd = "echo 10.4.0.0/14 | $rwsetbuild | $rwbagtool --intersect=- $file{v4bag2} | $rwbagcat";
my $md5 = "ebe316ba466c889ed61f531fac957731";

check_md5_output($md5, $cmd);
