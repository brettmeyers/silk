#! /usr/bin/perl -w
# MD5: 09d2ae140c19eea2b39e7b5c370b3f6e
# TEST: echo 10.0.0.0/8 | ./rwsetbuild | ./rwsetcat --net=v4:T,13,17,20/10,14,18

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwsetbuild = check_silk_app('rwsetbuild');
my $cmd = "echo 10.0.0.0/8 | $rwsetbuild | $rwsetcat --net=v4:T,13,17,20/10,14,18";
my $md5 = "09d2ae140c19eea2b39e7b5c370b3f6e";

check_md5_output($md5, $cmd);
