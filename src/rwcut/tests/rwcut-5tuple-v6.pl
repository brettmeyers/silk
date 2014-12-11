#! /usr/bin/perl -w
# MD5: e0e930f4274258d0907fcd4bfe3b6c68
# TEST: ./rwcut --fields=1-5 ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwcut --fields=1-5 $file{v6data}";
my $md5 = "e0e930f4274258d0907fcd4bfe3b6c68";

check_md5_output($md5, $cmd);
