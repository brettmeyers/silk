#! /usr/bin/perl -w
# MD5: 97c2a86022351bd60d2a37c70f03c218
# TEST: ./rwbagtool --subtract ../../tests/bag2-v6.bag ../../tests/bag1-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
$file{v6bag2} = get_data_or_exit77('v6bag2');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --subtract $file{v6bag2} $file{v6bag1} | $rwbagcat";
my $md5 = "97c2a86022351bd60d2a37c70f03c218";

check_md5_output($md5, $cmd);
