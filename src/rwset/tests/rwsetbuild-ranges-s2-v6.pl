#! /usr/bin/perl -w
# MD5: dff8da77c9b72348845517248346d89f
# TEST: ./rwsetcat --ip-ranges --delim=, ../../tests/set2-v6.set | cut -d, -f2,3 | ./rwsetbuild --ip-ranges=, | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --ip-ranges --delim=, $file{v6set2} | cut -d, -f2,3 | $rwsetbuild --ip-ranges=, | $rwsetcat --cidr";
my $md5 = "dff8da77c9b72348845517248346d89f";

check_md5_output($md5, $cmd);
