#! /usr/bin/perl -w
# MD5: 19dbae237ca68467c6b4bb32382cd8c8
# TEST: ./rwsetcat --network-structure=v6:60T,60/64,67,48,56 ../../tests/set2-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --network-structure=v6:60T,60/64,67,48,56 $file{v6set2}";
my $md5 = "19dbae237ca68467c6b4bb32382cd8c8";

check_md5_output($md5, $cmd);
