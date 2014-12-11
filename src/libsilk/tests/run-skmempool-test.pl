#! /usr/bin/perl -w
# MD5: 6c277f4ea8cfa4ec6331ee25c7e500b3
# TEST: ./skmempool-test 2>&1

use strict;
use SiLKTests;

my $skmempool_test = check_silk_app('skmempool-test');
my $cmd = "$skmempool_test 2>&1";
my $md5 = "6c277f4ea8cfa4ec6331ee25c7e500b3";

check_md5_output($md5, $cmd);
