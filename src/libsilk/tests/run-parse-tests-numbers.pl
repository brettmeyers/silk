#! /usr/bin/perl -w
# MD5: 90b48e92aa5049faf5bab6247919b540
# TEST: ./parse-tests --numbers 2>&1

use strict;
use SiLKTests;

my $parse_tests = check_silk_app('parse-tests');
my $cmd = "$parse_tests --numbers 2>&1";
my $md5 = "90b48e92aa5049faf5bab6247919b540";

check_md5_output($md5, $cmd);
