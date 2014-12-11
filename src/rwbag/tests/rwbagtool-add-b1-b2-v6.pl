#! /usr/bin/perl -w
# MD5: 2f4409041c8600e449f8d9b195251f4a
# TEST: ./rwbagtool --add ../../tests/bag1-v6.bag ../../tests/bag2-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
$file{v6bag2} = get_data_or_exit77('v6bag2');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --add $file{v6bag1} $file{v6bag2} | $rwbagcat";
my $md5 = "2f4409041c8600e449f8d9b195251f4a";

check_md5_output($md5, $cmd);
