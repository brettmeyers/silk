#! /usr/bin/perl -w
# MD5: 1a6d5cb7d97ee8acf500a92a153841fd
# TEST: ./parse-tests --dates 2>&1

use strict;
use SiLKTests;

my $parse_tests = check_silk_app('parse-tests');
my $cmd = "$parse_tests --dates 2>&1";
my $md5 = "1a6d5cb7d97ee8acf500a92a153841fd";

check_md5_output($md5, $cmd);
