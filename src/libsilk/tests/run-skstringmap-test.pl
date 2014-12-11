#! /usr/bin/perl -w
# MD5: 1fb8e1517f0e536dd611515a7129339f
# TEST: ./skstringmap-test 2>&1

use strict;
use SiLKTests;

my $skstringmap_test = check_silk_app('skstringmap-test');
my $cmd = "$skstringmap_test 2>&1";
my $md5 = "1fb8e1517f0e536dd611515a7129339f";

check_md5_output($md5, $cmd);
