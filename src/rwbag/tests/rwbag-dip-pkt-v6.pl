#! /usr/bin/perl -w
# MD5: 7eb936c9ca2ae2dd3cd3b8f6464195d9
# TEST: ./rwbag --dip-packets=stdout ../../tests/data-v6.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --dip-packets=stdout $file{v6data} | $rwbagcat";
my $md5 = "7eb936c9ca2ae2dd3cd3b8f6464195d9";

check_md5_output($md5, $cmd);
