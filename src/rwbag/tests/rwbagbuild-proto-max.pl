#! /usr/bin/perl -w
# MD5: 4c9708ca142efb0cca0460cf8709306c
# TEST: echo 255,100 | ./rwbagbuild --bag-input=stdin --delimiter=, --key-type=protocol --counter-type=sum-bytes | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "echo 255,100 | $rwbagbuild --bag-input=stdin --delimiter=, --key-type=protocol --counter-type=sum-bytes | $rwbagcat";
my $md5 = "4c9708ca142efb0cca0460cf8709306c";

check_md5_output($md5, $cmd);
