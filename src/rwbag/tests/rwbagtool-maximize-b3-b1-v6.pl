#! /usr/bin/perl -w
# MD5: f0249bb01eecae9f457b9f86fd0f9fd9
# TEST: ./rwbagtool --maximize ../../tests/bag3-v6.bag ../../tests/bag1-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
$file{v6bag3} = get_data_or_exit77('v6bag3');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --maximize $file{v6bag3} $file{v6bag1} | $rwbagcat";
my $md5 = "f0249bb01eecae9f457b9f86fd0f9fd9";

check_md5_output($md5, $cmd);
