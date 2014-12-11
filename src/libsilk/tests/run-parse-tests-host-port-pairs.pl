#! /usr/bin/perl -w
# MD5: varies
# TEST: ./parse-tests --host-port-pairs 2>&1

use strict;
use SiLKTests;

my $parse_tests = check_silk_app('parse-tests');
my $cmd = "$parse_tests --host-port-pairs 2>&1";
my $md5 = (($SiLKTests::SK_ENABLE_INET6_NETWORKING)
           ? "6e7d9fa2ed732c77ed8b56697e02c7d2"
           : "ec88cbd4066bc589b87eef9efe2cd7fe");
check_md5_output($md5, $cmd);
