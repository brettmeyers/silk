#! /usr/bin/perl -w
# MD5: 25f5aedf7e5c39d05ac838fd5c8dad5f
# TEST: ./parse-tests --tcp-flags 2>&1

use strict;
use SiLKTests;

my $parse_tests = check_silk_app('parse-tests');
my $cmd = "$parse_tests --tcp-flags 2>&1";
my $md5 = "25f5aedf7e5c39d05ac838fd5c8dad5f";

check_md5_output($md5, $cmd);
