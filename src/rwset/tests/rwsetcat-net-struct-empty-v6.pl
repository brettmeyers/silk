#! /usr/bin/perl -w
# MD5: 26d19348abd15dbd7bbadca3d1a8ba48
# TEST: ./rwsetbuild /dev/null | ./rwsetcat --net=v6:T,13,17,20/10,14,18

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwsetbuild = check_silk_app('rwsetbuild');
check_features(qw(ipset_v6));
my $cmd = "$rwsetbuild /dev/null | $rwsetcat --net=v6:T,13,17,20/10,14,18";
my $md5 = "26d19348abd15dbd7bbadca3d1a8ba48";

check_md5_output($md5, $cmd);
