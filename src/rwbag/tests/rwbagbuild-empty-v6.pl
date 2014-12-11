#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwbagbuild --bag-input=/dev/null --key-type=any-IPv6 --counter-type=sum-bytes | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwbagcat = check_silk_app('rwbagcat');
check_features(qw(ipv6));
my $cmd = "$rwbagbuild --bag-input=/dev/null --key-type=any-IPv6 --counter-type=sum-bytes | $rwbagcat";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
