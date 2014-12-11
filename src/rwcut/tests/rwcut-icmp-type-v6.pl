#! /usr/bin/perl -w
# MD5: 29bbe419f20d4bd309bf64d626e5ca22
# TEST: ../rwfilter/rwfilter --proto=58 --pass=- ../../tests/data-v6.rwf | ./rwcut --fields=4,5 --icmp-type-and-code

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwfilter --proto=58 --pass=- $file{v6data} | $rwcut --fields=4,5 --icmp-type-and-code";
my $md5 = "29bbe419f20d4bd309bf64d626e5ca22";

check_md5_output($md5, $cmd);
