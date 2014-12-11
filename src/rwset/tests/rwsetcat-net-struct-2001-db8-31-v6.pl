#! /usr/bin/perl -w
# MD5: 01ed6b42615ca300755578a202ac3b2f
# TEST: echo 2001:db8::/32 | ./rwsetbuild | ./rwsetcat --net=v6:ST,37,41,44,32/34,38,42,31

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwsetbuild = check_silk_app('rwsetbuild');
check_features(qw(ipset_v6));
my $cmd = "echo 2001:db8::/32 | $rwsetbuild | $rwsetcat --net=v6:ST,37,41,44,32/34,38,42,31";
my $md5 = "01ed6b42615ca300755578a202ac3b2f";

check_md5_output($md5, $cmd);
