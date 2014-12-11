#! /usr/bin/perl -w
# MD5: 119b469b50b05e45b5cc678d0758d561
# TEST: ./rwbagtool --compare=ge ../../tests/bag2-v6.bag ../../tests/bag1-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
$file{v6bag2} = get_data_or_exit77('v6bag2');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --compare=ge $file{v6bag2} $file{v6bag1} | $rwbagcat";
my $md5 = "119b469b50b05e45b5cc678d0758d561";

check_md5_output($md5, $cmd);
