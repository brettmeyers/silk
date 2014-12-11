#! /usr/bin/perl -w
# MD5: 1d1de3af120623a53952c2ff2eb1c3ff
# TEST: ./rwbag --dip-flows=stdout ../../tests/data-v6.rwf | ./rwbagcat --key-format=zero-padded

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --dip-flows=stdout $file{v6data} | $rwbagcat --key-format=zero-padded";
my $md5 = "1d1de3af120623a53952c2ff2eb1c3ff";

check_md5_output($md5, $cmd);
