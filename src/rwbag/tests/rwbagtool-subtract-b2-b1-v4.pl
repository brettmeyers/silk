#! /usr/bin/perl -w
# MD5: 0c820a4e62c1d80e2a3e25a5527f0386
# TEST: ./rwbagtool --subtract ../../tests/bag2-v4.bag ../../tests/bag1-v4.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v4bag1} = get_data_or_exit77('v4bag1');
$file{v4bag2} = get_data_or_exit77('v4bag2');
my $cmd = "$rwbagtool --subtract $file{v4bag2} $file{v4bag1} | $rwbagcat";
my $md5 = "0c820a4e62c1d80e2a3e25a5527f0386";

check_md5_output($md5, $cmd);
