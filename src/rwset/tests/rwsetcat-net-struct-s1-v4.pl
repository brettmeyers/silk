#! /usr/bin/perl -w
# MD5: 2a072173099d3171e5791865aeaaf9e5
# TEST: ./rwsetcat --network-structure ../../tests/set1-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsetcat --network-structure $file{v4set1}";
my $md5 = "2a072173099d3171e5791865aeaaf9e5";

check_md5_output($md5, $cmd);
