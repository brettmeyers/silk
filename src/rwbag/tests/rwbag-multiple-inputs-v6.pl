#! /usr/bin/perl -w
# MD5: 124cf19c2ca056abc26494ec2851533d
# TEST: ./rwbag --sport-flow=stdout ../../tests/empty.rwf ../../tests/data-v6.rwf ../../tests/data-v6.rwf | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{empty} = get_data_or_exit77('empty');
check_features(qw(ipv6));
my $cmd = "$rwbag --sport-flow=stdout $file{empty} $file{v6data} $file{v6data} | $rwbagcat --key-format=decimal";
my $md5 = "124cf19c2ca056abc26494ec2851533d";

check_md5_output($md5, $cmd);
