#! /usr/bin/perl -w
# MD5: 1e99be6d54e48fadffc9ff0e3cc69c88
# TEST: echo 2001:db8::/32 | ./rwsetbuild | ./rwsetcat --net=v6:T,37,41,44/34,38,42

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwsetbuild = check_silk_app('rwsetbuild');
check_features(qw(ipset_v6));
my $cmd = "echo 2001:db8::/32 | $rwsetbuild | $rwsetcat --net=v6:T,37,41,44/34,38,42";
my $md5 = "1e99be6d54e48fadffc9ff0e3cc69c88";

check_md5_output($md5, $cmd);
