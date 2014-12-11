#! /usr/bin/perl -w
# MD5: dff8da77c9b72348845517248346d89f
# TEST: ./rwsetcat --cidr-blocks ../../tests/set2-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --cidr-blocks $file{v6set2}";
my $md5 = "dff8da77c9b72348845517248346d89f";

check_md5_output($md5, $cmd);
