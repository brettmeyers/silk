#! /usr/bin/perl -w
# MD5: dba19381a3b2c2a84711370dfacd3a2b
# TEST: ./parse-tests --lists 2>&1

use strict;
use SiLKTests;

my $parse_tests = check_silk_app('parse-tests');
my $cmd = "$parse_tests --lists 2>&1";
my $md5 = "dba19381a3b2c2a84711370dfacd3a2b";

check_md5_output($md5, $cmd);
