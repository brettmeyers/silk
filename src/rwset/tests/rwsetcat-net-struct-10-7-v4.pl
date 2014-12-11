#! /usr/bin/perl -w
# MD5: ba679f5ec2ddd6eb5e4439432c498ae8
# TEST: echo 10.0.0.0/8 | ./rwsetbuild | ./rwsetcat --net=v4:ST,8,13,17,20/10,14,18,7

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwsetbuild = check_silk_app('rwsetbuild');
my $cmd = "echo 10.0.0.0/8 | $rwsetbuild | $rwsetcat --net=v4:ST,8,13,17,20/10,14,18,7";
my $md5 = "ba679f5ec2ddd6eb5e4439432c498ae8";

check_md5_output($md5, $cmd);
