#! /usr/bin/perl -w
# MD5: 2016747ec1c7dc07dae3cd6e3e0103ec
# TEST: ./rwsetcat --network-structure=v6: ../../tests/set1-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --network-structure=v6: $file{v6set1}";
my $md5 = "2016747ec1c7dc07dae3cd6e3e0103ec";

check_md5_output($md5, $cmd);
