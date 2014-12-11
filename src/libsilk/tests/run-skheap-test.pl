#! /usr/bin/perl -w
# MD5: 28fec1fb52c1b3e99f8510a9750b82a0
# TEST: ./skheap-test 2>&1

use strict;
use SiLKTests;

my $skheap_test = check_silk_app('skheap-test');
my $cmd = "$skheap_test 2>&1";
my $md5 = "28fec1fb52c1b3e99f8510a9750b82a0";

check_md5_output($md5, $cmd);
