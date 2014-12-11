#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwbagtool --add ../../tests/bag1-v6.bag ../../tests/bag2-v6.bag | ./rwbagtool --subtract - ../../tests/bag1-v6.bag ../../tests/bag2-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
$file{v6bag2} = get_data_or_exit77('v6bag2');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --add $file{v6bag1} $file{v6bag2} | $rwbagtool --subtract - $file{v6bag1} $file{v6bag2} | $rwbagcat";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
