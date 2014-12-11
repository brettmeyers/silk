#! /usr/bin/perl -w
# MD5: 8a5fd248c4bdeb265ed9f942a3678fae
# TEST: ./rwbagtool --scalar-multiply=2 ../../tests/bag1-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --scalar-multiply=2 $file{v6bag1} | $rwbagcat";
my $md5 = "8a5fd248c4bdeb265ed9f942a3678fae";

check_md5_output($md5, $cmd);
