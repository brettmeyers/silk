#! /usr/bin/perl -w
# MD5: 00991d26ba3bd723040655fcc847995b
# TEST: ./rwsetcat --network-structure=v6:18TS,18/48,67,56,64 ../../tests/set1-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --network-structure=v6:18TS,18/48,67,56,64 $file{v6set1}";
my $md5 = "00991d26ba3bd723040655fcc847995b";

check_md5_output($md5, $cmd);
