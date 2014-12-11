#! /usr/bin/perl -w
# MD5: f0cfebae5042c67f714e3c78f2a599f1
# TEST: ./rwset --sip-file=stdout ../../tests/data-v6.rwf | ./rwsetcat --network-structure=v6:

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --sip-file=stdout $file{v6data} | $rwsetcat --network-structure=v6:";
my $md5 = "f0cfebae5042c67f714e3c78f2a599f1";

check_md5_output($md5, $cmd);
