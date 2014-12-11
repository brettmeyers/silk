#! /usr/bin/perl -w
# MD5: 0e3f6d495aba5033d0ad1839dadba89e
# TEST: ./rwsetcat --cidr ../../tests/set1-v6.set | ./rwsetbuild | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --cidr $file{v6set1} | $rwsetbuild | $rwsetcat --cidr";
my $md5 = "0e3f6d495aba5033d0ad1839dadba89e";

check_md5_output($md5, $cmd);
