#! /usr/bin/perl -w
# MD5: bce1ff7003eea855683595703157cd75
# TEST: ./rwsetcat --network-structure=18TS,18 ../../tests/set1-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsetcat --network-structure=18TS,18 $file{v4set1}";
my $md5 = "bce1ff7003eea855683595703157cd75";

check_md5_output($md5, $cmd);
