#! /usr/bin/perl -w
# MD5: 475e203cfb6fa8d425d48271e930ed28
# TEST: ./hashlib_tests 2>&1

use strict;
use SiLKTests;

my $hashlib_tests = check_silk_app('hashlib_tests');
my $cmd = "$hashlib_tests 2>&1";
my $md5 = "475e203cfb6fa8d425d48271e930ed28";

check_md5_output($md5, $cmd);
