#! /usr/bin/perl -w
# MD5: 6b4da4c84dd83e00052f6b989fbfee80
# TEST: ./rwsetcat --count-ips ../../tests/set1-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --count-ips $file{v6set1}";
my $md5 = "6b4da4c84dd83e00052f6b989fbfee80";

check_md5_output($md5, $cmd);
