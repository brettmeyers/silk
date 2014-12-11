#! /usr/bin/perl -w
# MD5: 5a598d29841a76e937b01f10b25e42d2
# TEST: ./parse-tests --signals 2>&1

use strict;
use SiLKTests;

my $parse_tests = check_silk_app('parse-tests');
my $cmd = "$parse_tests --signals 2>&1";
my $md5 = "5a598d29841a76e937b01f10b25e42d2";

check_md5_output($md5, $cmd);
