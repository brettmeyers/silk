#! /usr/bin/perl -w
# MD5: 9b83d1b946f3925031d0ed007d2fcf33
# TEST: echo 256,100 | ./rwbagbuild --bag-input=stdin --delimiter=, --key-type=protocol --counter-type=sum-bytes | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "echo 256,100 | $rwbagbuild --bag-input=stdin --delimiter=, --key-type=protocol --counter-type=sum-bytes | $rwbagcat";
my $md5 = "9b83d1b946f3925031d0ed007d2fcf33";

check_md5_output($md5, $cmd);
