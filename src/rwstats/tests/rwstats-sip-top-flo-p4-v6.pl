#! /usr/bin/perl -w
# MD5: d887ec5cbd469181c5357f2d1d80ba6d
# TEST: ./rwstats --fields=sip --percentage=4 --top ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwstats --fields=sip --percentage=4 --top $file{v6data}";
my $md5 = "d887ec5cbd469181c5357f2d1d80ba6d";

check_md5_output($md5, $cmd);
