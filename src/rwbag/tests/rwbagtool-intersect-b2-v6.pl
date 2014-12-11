#! /usr/bin/perl -w
# MD5: 0b1be1bcaada57010df3015c118ecc86
# TEST: echo 2001:db8:a:4::/62 | ../rwset/rwsetbuild | ./rwbagtool --intersect=- ../../tests/bag2-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag2} = get_data_or_exit77('v6bag2');
check_features(qw(ipset_v6));
my $cmd = "echo 2001:db8:a:4::/62 | $rwsetbuild | $rwbagtool --intersect=- $file{v6bag2} | $rwbagcat";
my $md5 = "0b1be1bcaada57010df3015c118ecc86";

check_md5_output($md5, $cmd);
