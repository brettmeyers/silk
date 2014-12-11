#! /usr/bin/perl -w
# MD5: varies
# TEST: ./skbitmap-test 2>&1

use strict;
use SiLKTests;

my $skbitmap_test = check_silk_app('skbitmap-test');
my $cmd = "$skbitmap_test 2>&1";
my $md5 = (($SiLKTests::SK_ENABLE_IPV6)
           ? "7354cd57410e7c8efaa07d09c85bd797"
           : "dcfcdc31fc5678bd8cd5fcbcf86bf0cf");

check_md5_output($md5, $cmd);
