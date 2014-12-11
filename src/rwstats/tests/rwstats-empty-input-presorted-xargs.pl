#! /usr/bin/perl -w
# MD5: 801e3a96fd06d45f22ed09e2d5fa871f
# TEST: cat /dev/null | ./rwstats --fields=dip --count=10 --top --ipv6-policy=ignore --presorted-input --xargs=-

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $cmd = "cat /dev/null | $rwstats --fields=dip --count=10 --top --ipv6-policy=ignore --presorted-input --xargs=-";
my $md5 = "801e3a96fd06d45f22ed09e2d5fa871f";

check_md5_output($md5, $cmd);
