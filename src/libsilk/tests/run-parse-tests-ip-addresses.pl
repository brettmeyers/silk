#! /usr/bin/perl -w
# MD5: varies
# TEST: ./parse-tests --ip-addresses 2>&1

use strict;
use SiLKTests;

my $parse_tests = check_silk_app('parse-tests');
my $cmd = "$parse_tests --ip-addresses 2>&1";
my $md5 = (($SiLKTests::SK_ENABLE_IPV6)
           ? "8c8a2a95e38b6837e575e9de639bc72b"
           : "83216c120d3726169db40eb5435e06cb");
check_md5_output($md5, $cmd);
