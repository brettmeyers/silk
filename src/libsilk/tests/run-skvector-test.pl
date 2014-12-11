#! /usr/bin/perl -w
# MD5: 0095b5c884adc4e718d64268e7c95f91
# TEST: ./skvector-test 2>&1

use strict;
use SiLKTests;

my $skvector_test = check_silk_app('skvector-test');
my $cmd = "$skvector_test 2>&1";
my $md5 = "0095b5c884adc4e718d64268e7c95f91";

check_md5_output($md5, $cmd);
