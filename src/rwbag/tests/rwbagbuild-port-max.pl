#! /usr/bin/perl -w
# MD5: d558620067865abf4f577eab04a469ec
# TEST: echo 65535,100 | ./rwbagbuild --bag-input=stdin --delimiter=, --key-type=sport --counter-type=sum-bytes | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "echo 65535,100 | $rwbagbuild --bag-input=stdin --delimiter=, --key-type=sport --counter-type=sum-bytes | $rwbagcat";
my $md5 = "d558620067865abf4f577eab04a469ec";

check_md5_output($md5, $cmd);
