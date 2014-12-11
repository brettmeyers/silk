#! /usr/bin/perl -w
# MD5: 2684cdfcf12338d8ac00883d5994210d
# TEST: echo 2001:db8:a:4::/62 | ../rwset/rwsetbuild | ./rwbagtool --complement-intersect=- ../../tests/bag2-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag2} = get_data_or_exit77('v6bag2');
check_features(qw(ipset_v6));
my $cmd = "echo 2001:db8:a:4::/62 | $rwsetbuild | $rwbagtool --complement-intersect=- $file{v6bag2} | $rwbagcat";
my $md5 = "2684cdfcf12338d8ac00883d5994210d";

check_md5_output($md5, $cmd);
