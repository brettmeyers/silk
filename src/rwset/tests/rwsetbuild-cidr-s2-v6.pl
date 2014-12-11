#! /usr/bin/perl -w
# MD5: dff8da77c9b72348845517248346d89f
# TEST: ./rwsetcat --cidr ../../tests/set2-v6.set | ./rwsetbuild | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --cidr $file{v6set2} | $rwsetbuild | $rwsetcat --cidr";
my $md5 = "dff8da77c9b72348845517248346d89f";

check_md5_output($md5, $cmd);
